#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif
#include "../../common/CommonDefines.h"
#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#ifdef NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#ifdef ENABLE_BIOLOG_SYSTEM
	#include "biolog.h"
#endif
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#ifdef OFFLINE_SHOP
#include "offlineshop_manager.h"
#endif
#include "threeway_war.h"
#include "log.h"
#include "../../common/VnumHelper.h"
#ifdef __AUCTION__
#include "auction_manager.h"
#endif
#ifdef ENABLE_DECORUM
#include "decorum_manager.h"
#include "decorum_arena.h"
#endif
#ifdef __WORLD_BOSS_YUMA__
#include "worldboss.h"
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
ACMD(do_debug_anti_multiple_farm)
{
	LPDESC d = nullptr;
	if (!ch || (ch && !(d = ch->GetDesc())))
		return;

	CAntiMultipleFarm::instance().PrintPlayerDropState(d->GetLoginMacAdress(), ch);
}
#endif


#ifdef M2S_BIO_SYSTEM
ACMD(do_bio_mission)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	int slot;
	str_to_number(slot, arg1);

	ch->UpdateBioProcess(slot);
}
#endif

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		// 말이 아닌 다른탈것을 타고있다.
		if (ch->GetMountVnum())
		{

			LPITEM item = ch->GetWear(WEAR_COSTUME_MOUNT);

			if (item && item->IsRideItem())
				ch->UnequipItem(item);
	
			if (ch->UnEquipSpecialRideUniqueItem())
			{
				ch->RemoveAffect(AFFECT_MOUNT);
				ch->RemoveAffect(AFFECT_MOUNT_BONUS);
			}
			
			//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"AI¹I A≫°IA≫ AI¿eAßAO´I´U."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi prima evocare una cavalcatura."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_capitale_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi ancora visitare la Capitale."));
		return;
	}

	ch->WarpSet(989000, 281300);
}

ACMD(do_valle_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 20)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 20."));
		return;
	}

	ch->WarpSet(333300, 748300);
}

ACMD(do_covo_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 30)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 30."));
		return;
	}

	ch->WarpSet(704000, 464600);
}

ACMD(do_eruzione_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 100)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 90."));
		return;
	}

	ch->WarpSet(2600200, 2272800);
}

ACMD(do_monte_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 30)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 30."));
		return;
	}

	ch->WarpSet(434700, 214200);
}

ACMD(do_deserto_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 30)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 30."));
		return;
	}

	ch->WarpSet(291500, 549600);
}

ACMD(do_grotta_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 75)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 75."));
		return;
	}

	ch->WarpSet(241600, 1273000);
}

ACMD(do_capo_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 90)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 90."));
		return;
	}

	ch->WarpSet(731500, 2300300);
}

ACMD(do_bosco_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 50)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 50."));
		return;
	}

	ch->WarpSet(1119300, 70700);
}

ACMD(do_fantasma_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 50)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 50."));
		return;
	}

	ch->WarpSet(287300, 42700);
}

ACMD(do_fuoco_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 50)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 50."));
		return;
	}

	ch->WarpSet(604200, 685300);
}

ACMD(do_baia_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 90)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 90."));
		return;
	}

	ch->WarpSet(1088100, 1649700);
}

ACMD(do_tonanti_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 110)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 110."));
		return;
	}

	ch->WarpSet(1240600, 2320100);
}

ACMD(do_torre_teleport)
{
	if (!ch)
		return;
	
	if (ch->GetLevel() < 40)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi essere almeno livello 40."));
		return;
	}

	ch->WarpSet(216300, 727800);
}



ACMD(do_user_horse_back)
{
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
	if (mount) {
		ch->UnequipItem(mount);
	}
	else {
		if (ch->GetHorse() != NULL)
		{
			ch->HorseSummon(false);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말을 돌려보냈습니다."));
		}
		else if (ch->IsHorseRiding() == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말에서 먼저 내려야 합니다."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi prima evocare una cavalcatura."));
		}
	}
#else
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말을 돌려보냈습니다."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말에서 먼저 내려야 합니다."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi prima evocare una cavalcatura."));
	}
#endif
}

ACMD(do_user_horse_feed)
{
	// 개인상점을 연 상태에서는 말 먹이를 줄 수 없다.
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi prima evocare una cavalcatura."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말을 탄 상태에서는 먹이를 줄 수 없습니다."));
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"말에게 %s%s 주었습니다."),
				#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
				ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName[ch->GetLanguage()],
				g_iUseLocale ? "" : under_han(ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName[ch->GetLanguage()]) ? LC_TEXT("을") : LC_TEXT("를"));
				#else
				ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName,
				"");
				#endif
	}
	else
	{
		#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 아이템이 필요합니다"), ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName[ch->GetLanguage()]);
		#else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 아이템이 필요합니다"), ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName);
		#endif
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int         	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo()
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

/* struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
	: seconds( 0 )
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("셧다운이 %d초 남았습니다."), *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();
#ifdef ENABLE_DECORUM
	CDecoredArenaManager::instance().EndAllDecoredArena();
#endif
	char buf[64];
	snprintf(buf, sizeof(buf), LC_TEXT("%d초 후 게임이 셧다운 됩니다."), iSec);

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}
	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}
*/
EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );

	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
			case SCMD_QUIT:
			case SCMD_PHASE_SELECT:
				{
					TPacketNeedLoginLogInfo acc_info;
					acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;

					db_clientdesc->DBPacket( HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info) );

					LogManager::instance().DetailLoginLog( false, ch );
				}
				break;
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				break;

			case SCMD_PHASE_SELECT:
				{
					ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

					if (d)
					{
						d->SetPhase(PHASE_SELECT);
					}
				}
				break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%d초 남았습니다."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	/* RECALL_DELAY
	   if (ch->m_pkRecallEvent != NULL)
	   {
	   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"취소 되었습니다."));
	   event_cancel(&ch->m_pkRecallEvent);
	   return;
	   }
	// END_OF_RECALL_DELAY */

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"취소 되었습니다."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"로그인 화면으로 돌아 갑니다. 잠시만 기다리세요."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"게임을 종료 합니다. 잠시만 기다리세요."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"캐릭터를 전환 합니다. 잠시만 기다리세요."));
			break;
	}

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

				{
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
						info->left_second = 3;
				}

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

/* ACMD(do_daily_reward_reload){
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem DeleteRewards|");
	char* time = "";
	char* rewards = "";
	char* items;
	char* counts;
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time),reward from player.daily_reward_status where pid = %u",ch->GetPlayerID()));
	SQLResult * pRes = pkMsg->Get();

	if (pRes->uiNumRows > 0){
		SQLMsg * pkMsg9(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time),reward from player.daily_reward_status where pid = %u and (time + INTERVAL 1 DAY < NOW()) limit 1;", ch->GetPlayerID()));
		SQLResult * pRes9 = pkMsg9->Get();
		if (pRes9->uiNumRows > 0){
			DBManager::Instance().DirectQuery("DELETE FROM player.daily_reward_status where pid = %u", ch->GetPlayerID());
			DBManager::Instance().DirectQuery("INSERT INTO player.daily_reward_status (pid,time,reward,total_rewards) VALUES(%u,NOW(),0,0)", ch->GetPlayerID());
			ch->ChatPacket(CHAT_TYPE_INFO, "You didnt take the item in the last 24h, so the reward will be the first one.");
			SQLMsg * pkMsg8(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time), reward from player.daily_reward_status where pid = %u", ch->GetPlayerID()));
			SQLResult * pRes8 = pkMsg8->Get();
			if (pRes8->uiNumRows > 0){
				MYSQL_ROW row;
				while ((row = mysql_fetch_row(pRes8->pSQLResult)) != NULL){
					time = row[0];
					rewards = row[1];
				}
			}
		}
		else{
			SQLMsg * pkMsg2(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time), reward from player.daily_reward_status where pid = %u", ch->GetPlayerID()));
			SQLResult * pRes2 = pkMsg2->Get();
			if (pRes2->uiNumRows > 0){
				MYSQL_ROW row;
				while ((row = mysql_fetch_row(pRes2->pSQLResult)) != NULL){
					time = row[0];
					rewards = row[1];
				}
			}
		}
	}
	else{
		DBManager::Instance().DirectQuery("INSERT INTO player.daily_reward_status (pid,time,reward,total_rewards) VALUES(%u,NOW(),0,0)", ch->GetPlayerID());
		SQLMsg * pkMsg2(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time), reward from player.daily_reward_status where pid = %u",ch->GetPlayerID()));
		SQLResult * pRes2 = pkMsg2->Get();
		if (pRes2->uiNumRows > 0){
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes2->pSQLResult)) != NULL){
				time = row[0];
				rewards = row[1];
			}
		}
	}

	SQLMsg * pkMsg3(DBManager::instance().DirectQuery("SELECT items, count from player.daily_reward_items where reward = '%s'",rewards));
	SQLResult * pRes3 = pkMsg3->Get();
	if (pRes3->uiNumRows > 0){
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes3->pSQLResult)) != NULL){
			items = row[0];
			counts = row[1];
			ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetReward|%s|%s",items,counts);
		}
	}
	
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetTime|%s",time);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetDailyReward|%s", rewards);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetRewardDone|");
}

ACMD(do_daily_reward_get_reward){
	if (!ch)
		return;

	char* items;
	char* counts;
	DWORD item;
	DWORD count;
	bool reward = false;
	char* rewards;
	// and (NOW() - interval 30 minute > time) 
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT reward from player.daily_reward_status where (NOW() > time) and pid = %u", ch->GetPlayerID()));
	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0){
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL){
			rewards = row[0];
		}
		reward = true;
	}
	
	if (reward){
		SQLMsg * pkMsg2(DBManager::instance().DirectQuery("SELECT items, count from player.daily_reward_items where reward = '%s' ORDER BY RAND() limit 1",rewards));
		SQLResult * pRes2 = pkMsg2->Get();
		if (pRes2->uiNumRows > 0){
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes2->pSQLResult)) != NULL){
				items = row[0];
				counts = row[1];
			}
		}
		str_to_number(item, items);
		str_to_number(count, counts);
		ch->AutoGiveItem(item, count);
		// ch->ChatPacket(CHAT_TYPE_INFO, "recompensa: %s",items);
		DBManager::Instance().DirectQuery("UPDATE daily_reward_status SET reward = CASE WHEN reward = 0 THEN '1' WHEN reward = 1 THEN '2' WHEN reward = 2 THEN '3' WHEN reward = 3 THEN '4' WHEN reward = 4 THEN '5' WHEN reward = 5 THEN '6' WHEN reward = 6 THEN '0' END, total_rewards = total_rewards +1, time = (NOW() + interval 1 day) WHERE pid = %u", ch->GetPlayerID());
	}
	else{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "You can't get the reward yet."));
	}
}
 */
ACMD(do_mount)
{
	/*
	   char			arg1[256];
	   struct action_mount_param	param;

	// 이미 타고 있으면
	if (ch->GetMountingChr())
	{
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	return;

	param.x		= atoi(arg1);
	param.y		= atoi(arg2);
	param.vid	= ch->GetMountingChr()->GetVID();
	param.is_unmount = true;

	float distance = DISTANCE_SQRT(param.x - (DWORD) ch->GetX(), param.y - (DWORD) ch->GetY());

	if (distance > 600.0f)
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"좀 더 가까이 가서 내리세요."));
	return;
	}

	action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
	return;
	}

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	return;

	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(atoi(arg1));

	if (!tch->IsNPC() || !tch->IsMountable())
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"거기에는 탈 수 없어요."));
	return;
	}

	float distance = DISTANCE_SQRT(tch->GetX() - ch->GetX(), tch->GetY() - ch->GetY());

	if (distance > 600.0f)
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"좀 더 가까이 가서 타세요."));
	return;
	}

	param.vid		= tch->GetVID();
	param.is_unmount	= false;

	action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
	 */
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

#ifdef __WORLD_BOSS_YUMA__
ACMD(do_boss_debug)
{
	CWorldBossManager::instance().GetWorldbossInfo(ch);
}
#endif


#ifdef ENABLE_BIOLOG_SYSTEM
ACMD(do_biolog)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	
	if (!*arg1)
	{
		BiologManager::instance().SendButton(ch);
		return;
	}	
	
	if (!strcmp(arg1, "92_reward_1")) {
		BiologManager::instance().SelectBonusType(ch, "92_reward_1"); return; }		
	if (!strcmp(arg1, "92_reward_2")) {
		BiologManager::instance().SelectBonusType(ch, "92_reward_2"); return; }				
	if (!strcmp(arg1, "92_reward_3")) {
		BiologManager::instance().SelectBonusType(ch, "92_reward_3"); return; }		
	if (!strcmp(arg1, "94_reward_1")) {
		BiologManager::instance().SelectBonusType(ch, "94_reward_1"); return; }			
	if (!strcmp(arg1, "94_reward_2")) {
		BiologManager::instance().SelectBonusType(ch, "94_reward_2"); return; }					
	if (!strcmp(arg1, "94_reward_3")) {
		BiologManager::instance().SelectBonusType(ch, "94_reward_3"); return; }			
}	
#endif

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		if (!test_server)
		{
			if (ch->IsHack())
			{
				//성지 맵일경우에는 체크 하지 않는다.
				if (false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - (170 - g_nPortalLimitTime));
					return;
				}
			}
#define eFRS_HERESEC	175
			if (iTimeToDead > eFRS_HERESEC)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - eFRS_HERESEC);
				return;
			}
		}
	}

	//PREVENT_HACK
	//DESC : 창고, 교환 창 후 포탈을 사용하는 버그에 이용될수 있어서
	//		쿨타임을 추가
	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (ch->IsHack())
		{
			//길드맵, 성지맵에서는 체크 하지 않는다.
			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG) ||
			   	false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"아직 재시작 할 수 없습니다. (%d초 남음)"), iTimeToDead - (170 - g_nPortalLimitTime));
				return;
			}
		}

#define eFRS_TOWNSEC	180
		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"아직 마을에서 재시작 할 수 없습니다. (%d 초 남음)"), iTimeToDead - eFRS_TOWNSEC);
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	//FORKED_LOAD
	//DESC: 삼거리 전투시 부활을 할경우 맵의 입구가 아닌 삼거리 전투의 시작지점으로 이동하게 된다.
	if (1 == quest::CQuestManager::instance().GetEventFlag("threeway_war"))
	{
		if (subcmd == SCMD_RESTART_TOWN || subcmd == SCMD_RESTART_HERE)
		{
			if (true == CThreeWayWar::instance().IsThreeWayWarMapIndex(ch->GetMapIndex()) &&
					false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

				ch->ReviveInvisible(5);
				ch->PointChange(POINT_HP, 5000 + ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

				return;
			}

			//성지
			if (true == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				if (CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()) <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"성지에서 부활 기회를 모두 잃었습니다! 마을로 이동합니다!"));
					ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
				{
					ch->Show(ch->GetMapIndex(), GetSungziStartX(ch->GetEmpire()), GetSungziStartY(ch->GetEmpire()));
				}

				ch->PointChange(POINT_HP, 5000 + ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);

				return;
			}
		}
	}
	//END_FORKED_LOAD

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, 5000 + ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif
					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
					ch->PointChange(POINT_HP, 5000 + ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					ch->CheckMount();
#endif
					break;
			}

			return;
		}
	}
	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;

			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			ch->PointChange(POINT_HP, 5000 + ch->GetHP());
			ch->DeathPenalty(1);
			break;

		case SCMD_RESTART_HERE:
			sys_log(0, "do_restart: restart here");
			ch->RestartAtSamePos();
			//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
			ch->PointChange(POINT_HP, 5000 + ch->GetHP());
			ch->DeathPenalty(0);
			ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			ch->CheckMount();
#endif
			break;
	}
#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena() && !ch->IsObserverMode())
	{
		switch (subcmd)
		{
			case SCMD_RESTART_TOWN:
				sys_log(0, "do_restart: restart town GetDecorumArena %s", ch->GetName());
				ch->GoHomeFromArena();
				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				break;

			case SCMD_RESTART_HERE:
				sys_log(0, "do_restart: restart here GetDecorumArena %s", ch->GetName());
				ch->GetDecorumArena()->WarpToStartPosition(ch);
				ch->ReviveInvisible(5);
				break;
				
		}
		return;
	}
#endif	
}

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

#ifdef __ENABLE_TRASH_BIN__
ACMD(do_trash_bin)
{
	if(!ch)
		return;
	
	if(ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("<System> Nu se poate efectua actiunea."));
		return;
	}
		
	char arg1[30]; //item pos
	char arg2[30]; //item vnum
	char arg3[30]; //item count
	
	//get info
	argument = one_argument(argument,arg1,sizeof(arg1));
	argument = one_argument(argument,arg2,sizeof(arg2));
	argument = one_argument(argument,arg3,sizeof(arg3));
	
	
	//check string argument
	if(arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
	{
		sys_err("INVALID ARGUMENT FOR COMMAND do_trash_bin FROM PLAYER [%s]",ch->GetName());
		return;
	}
	
	DWORD dwItemVnum = 0 , dwItemCount = 0;
	int iItemPos = 0;
	
	//cast info into number
	str_to_number(iItemPos ,	arg1);
	str_to_number(dwItemVnum,	arg2);
	str_to_number(dwItemCount,	arg3);
	
	
	//check for invalid info
	if( 0 == dwItemVnum || 0 == dwItemCount)
	{
		sys_err("INVALID ARGUMENT FOR COMMAND do_trash_bin FROM PLAYER[%s] count [%d] vnum[%d]",ch->GetName() ,dwItemCount,dwItemVnum);
		return;
	}
	
	
	//check for black_list
	if(std::find(g_vec_trash_bin_black_list.begin() , g_vec_trash_bin_black_list.end() , dwItemVnum) != g_vec_trash_bin_black_list.end())
	{
		sys_err("CANNOT DESTROY ITEM [%d] BLACK LIST VIOLATION FROM PLAYER [%s]",dwItemVnum,ch->GetName());
		return;
	}
	
	
	int iRewardCount = 0;
	
	
	//check for reward list
	itertype(g_map_trash_bin_reward) it = g_map_trash_bin_reward.find(dwItemVnum);
	
	if(it != g_map_trash_bin_reward.end())
		iRewardCount = it->second;
	
	
	//destroy item
	LPITEM item = ch->GetInventoryItem(iItemPos);
	
	if(!item)
		return;
	
	if(item->GetVnum() != dwItemVnum || item->GetCount() != dwItemCount)
		return;
	
	ITEM_MANAGER::instance().RemoveItem(item,"TRASH_BIN");
	
	//reward
	if(iRewardCount > 0)
	{
		ch->AutoGiveItem(TRASH_BIN_REWARD_ITEM_VNUM, iRewardCount, 0, true);
	}
}
#endif

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	// ch->ChatPacket(CHAT_TYPE_INFO, "%s GRP(%d) idx(%u), MAX_STAT(%d), expr(%d)", __FUNCTION__, ch->GetRealPoint(idx), idx, MAX_STAT, ch->GetRealPoint(idx) >= MAX_STAT);
	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -1);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena() != NULL || CDecoredArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "You cannot use this in the Arena."));
		return;
	}
#endif	
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"대련장에서 사용하실 수 없습니다."));
		return;
	}

#ifdef ENABLE_RENEWAL_PVP
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { return; }

	DWORD vid = 0;
	str_to_number(vid, vecArgs[2].c_str());

	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);
	if (!pkVictim)
		return;
	if (pkVictim->IsNPC())
		return;
	
#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().CheckMessengerList(ch->GetName(), pkVictim->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s blokkk"), pkVictim->GetName());
		return;
	}
#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(pkVictim->GetLanguage(),"상대방이 대련중입니다."));
		return;
	}

#ifdef ENABLE_DECORUM
	if (pkVictim->GetDecorumArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(pkVictim->GetLanguage(), "상대방이 대련중입니다."));
		return;
	}

	bool bIsDecored = false;
	if (vecArgs[1] == "decorum")
	{
		DECORUM * pChDec = CDecorumManager::instance().GetDecorum(ch->GetPlayerID());
		DECORUM * pVickDec = CDecorumManager::instance().GetDecorum(pkVictim->GetPlayerID());
		if (!pChDec || !pVickDec || !pChDec->IsDecored() || !pVickDec->IsDecored())
			return;
		if (pVickDec->IsBlock(BLOCK_DUELL))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Lega> L'avversario ha rifiutato il duello eroe."));
			return;
		}
		if(pkVictim->GetLevel() > ch->GetLevel() + MAX_LEVEL_RANGE_LOBBY_APPLICANT || pkVictim->GetLevel() < ch->GetLevel() - MIN_LEVEL_RANGE_LOBBY_APPLICANT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Lega> Il tuo livello e' troppo differente dall'avversario."));
			return;
		}

		bIsDecored = true;
		long long betPrice = 0;
		bool pvpData[PVP_BET];
		CPVPManager::instance().Insert(ch, pkVictim, bIsDecored, pvpData, betPrice);
	}
#endif
	else if (vecArgs[1] == "revenge")
	{
		if (!CPVPManager::Instance().HasPvP(ch, pkVictim))
			return;
		long long betPrice = 0;
		bool pvpData[PVP_BET];
#ifdef ENABLE_DECORUM
		CPVPManager::instance().Insert(ch, pkVictim, bIsDecored, pvpData, betPrice);
#else
		CPVPManager::instance().Insert(ch, pkVictim, pvpData, betPrice);
#endif
	}
	else if (vecArgs[1] == "pvp")
	{
		if (vecArgs.size() != 28) { return; }

		long long betPrice = 0;
		bool pvpData[PVP_BET];
		memset(&pvpData, false, sizeof(pvpData));

		for (DWORD j = 3; j < vecArgs.size() - 1; ++j)
			pvpData[j - 3] = vecArgs[j] == "1" ? true : false;
		str_to_number(betPrice, vecArgs[vecArgs.size() - 1].c_str());

		if (CPVPManager::Instance().IsFighting(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1001"));
			return;
		}
		else if (CPVPManager::Instance().IsFighting(pkVictim))
		{
			ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1002"));
			return;
		}
		else if (betPrice > ch->GetGold() || betPrice >= GOLD_MAX || ch->GetGold()+betPrice >= GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1000"));
			return;
		}
		else if (betPrice > pkVictim->GetGold() || betPrice >= GOLD_MAX || ch->GetGold()+betPrice >= GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1003"));
			return;
		}

#ifdef ENABLE_DECORUM
		CPVPManager::instance().Insert(ch, pkVictim, bIsDecored, pvpData, betPrice);
#else
		CPVPManager::instance().Insert(ch, pkVictim, pvpData, betPrice);
#endif
	}
	else if (vecArgs[1] == "close")
	{
		CPVPManager::instance().RemoveCharactersPvP(ch, pkVictim);
	}
#else


#ifdef ENABLE_DECORUM
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
#else
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
#endif

#ifdef ENABLE_DECORUM
	bool bIsDecored = false;
	if (*arg2)
		bIsDecored = true;
#endif

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;
	#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().CheckMessengerList(ch->GetName(), pkVictim->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s blokkk"), pkVictim->GetName());
		return;
	}
	#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(pkVictim->GetLanguage(),"상대방이 대련중입니다."));
		return;
	}

#ifdef ENABLE_DECORUM
	if (bIsDecored)
	{
		DECORUM * pChDec = CDecorumManager::instance().GetDecorum(ch->GetPlayerID());
		DECORUM * pVickDec = CDecorumManager::instance().GetDecorum(pkVictim->GetPlayerID());
		
		if (!pChDec || !pVickDec || !pChDec->IsDecored() || !pVickDec->IsDecored())
			return;
			
		if (pVickDec->IsBlock(BLOCK_DUELL))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Lega> L'avversario ha rifiutato il duello eroe."));
			return;
		}
		
		if(pkVictim->GetLevel() > ch->GetLevel() + MAX_LEVEL_RANGE_LOBBY_APPLICANT || pkVictim->GetLevel() < ch->GetLevel() - MIN_LEVEL_RANGE_LOBBY_APPLICANT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "<Lega> Il tuo livello e' troppo differente dall'avversario.""));
			return;
		}
	}

	if (pkVictim->GetDecorumArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(pkVictim->GetLanguage(), "상대방이 대련중입니다."));
		return;
	}
#endif

#ifdef ENABLE_DECORUM
	CPVPManager::instance().Insert(ch, pkVictim, bIsDecored);
#else
	CPVPManager::instance().Insert(ch, pkVictim);
#endif
#endif
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드에 속해있지 않습니다."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드 스킬 레벨을 변경할 권한이 없습니다."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
// @version	05/06/20 Bang2ni - 커맨드 처리 Delegate to CHARACTER class
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
// @version	05/06/20 Bang2ni - 커맨드 처리 Delegate to CHARACTER class
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 잘못된 암호를 입력하셨습니다."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 창고가 이미 열려있습니다."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10) // 10초에 한번만 요청 가능
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 창고를 닫은지 10초 안에는 열 수 없습니다."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
ACMD (do_costume_hide)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));
	
	if (0 == arg1[0])
		return;

	int slot = atoi(arg1);

	ch->FuncHideCostume(slot);
}
#endif

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 던전 안에서는 파티에서 나갈 수 없습니다."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 파티에서 나가셨습니다."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}


ACMD(do_close_shop)
{
	if (ch->IsObserverMode())
		return;
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}


ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_ch)
{
    char arg1[256];
    one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1)
        return;
 
    int new_ch;
    str_to_number(new_ch, arg1);

    if( new_ch <1 || new_ch >4)
        return;

    if (!ch->IsPC())
        return;

	// PREVENT BUG WHILE EDIT MODE
	if (ch->IsOwnerEditShopOffline())
		return;
	// PREVENT BUG WHILE EDIT MODE

    ch->ChannelSwitch(new_ch);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

#if defined(ENABLE_AFFECT_POLYMORPH_REMOVE)
ACMD(do_remove_polymorph)
{
	if (!ch)
		return;
	
	if (!ch->IsPolymorphed())
		return;
	
	ch->SetPolymorph(0);
	ch->RemoveAffect(AFFECT_POLYMORPH);
}
#endif

ACMD(do_war)
{
	
	CGuild * g = ch->GetGuild();

	if (!g)
		return;

	
	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Breasla ta participa la un alt razboi."));
		return;
	}

#ifdef __IMPROVED_GUILD_WAR__
	const char *line;
	char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], arg6[256];
	line = one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));
	one_argument(two_arguments(line, arg4, sizeof(arg4), arg5, sizeof(arg5)), arg6, sizeof(arg6));
#else
	char arg1[256], arg2[256];
	// DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
#endif

	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type < 0) {	//@fixme441
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<error> can't declare war with type less than zero."));
			return;
		}

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	DWORD gm_pid = g->GetMasterPID();
#ifdef ENABLE_GENERAL_IN_GUILD
	if (gm_pid != ch->GetPlayerID())
	{
		LPCHARACTER general = CHARACTER_MANAGER::Instance().FindByPID(g->GetGeneralMember());
		if(ch == general)
		{
			// if leader online return general command.
			if(g->GetMasterCharacter() != NULL)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Non puoi farlo perche' il leader e' online!");
				return;
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
			return;
		}
	}
#else
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}
#endif
	
	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Aceasta breasla nu exista."));
		return;
	}

	//ch->ChatPacket(CHAT_TYPE_INFO, "guild %d->%d state %d", g->GetID(), opp_g->GetID(), g->GetGuildWarState(opp_g->GetID()));
	
	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드가 이미 전쟁 중 입니다."));
					return;
				}

				int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 전비가 부족하여 길드전을 할 수 없습니다."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 선전포고 중인 길드입니다."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Breasla> Aceasta breasla este deja in razboi."));
					g->RequestRefuseWar(opp_g->GetID()
#ifdef __IMPROVED_GUILD_WAR__			
						, 0, 0, 0, 0
#endif
					);
					
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 전쟁이 예약된 길드 입니다."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Breasla> Aceasta breasla este deja in razboi."));
			g->RequestRefuseWar(opp_g->GetID()
#ifdef __IMPROVED_GUILD_WAR__			
				, 0, 0, 0, 0
#endif
			);
			return;
	}

	if (!g->CanStartWar(type))
	{
		
#ifndef GUILD_RANK_EFFECT
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "361");
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
#endif
		if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Breasla trebuie sa aiba cel putin 3 membri pentru a participa la un razboi."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	
	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		//if (opp_g->GetLadderPoint() == 0)
		//	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방 길드의 레더 점수가 모자라서 길드전을 할 수 없습니다."));
		if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Breasla nu are suficienti membri pentru a participa la Razboiul Breaslei."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;
		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());
		if (pCCI != NULL)
			break;

#ifdef ENABLE_GENERAL_IN_GUILD
		DWORD generalPID = g->GetGeneralMember();
		if(generalPID != 0)
		{
			LPCHARACTER general = CHARACTER_MANAGER::Instance().FindByPID(generalPID);
			if(general != NULL)
				break;
			pCCI = P2P_MANAGER::instance().FindByPID(generalPID);
			if (pCCI != NULL)
				break;
		}
#endif

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Liderul advers este offline."));
		g->RequestRefuseWar(opp_g->GetID()
#ifdef __IMPROVED_GUILD_WAR__			
			, 0, 0, 0, 0
#endif
		);
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;
		
		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());
		
		if (pCCI != NULL)
			break;

#ifdef ENABLE_GENERAL_IN_GUILD
		DWORD generalPID = opp_g->GetGeneralMember();
		if(generalPID != 0)
		{
			LPCHARACTER general = CHARACTER_MANAGER::Instance().FindByPID(generalPID);
			if(general != NULL)
				break;
			pCCI = P2P_MANAGER::instance().FindByPID(generalPID);
			if (pCCI != NULL)
				break;
		}
#endif

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Liderul advers este offline."));
		g->RequestRefuseWar(opp_g->GetID()
#ifdef __IMPROVED_GUILD_WAR__			
			, 0, 0, 0, 0
#endif		
		);
		return;


	} while (false);

#ifdef __IMPROVED_GUILD_WAR__
	int iMaxPlayer = 0;
	if (!*arg3) iMaxPlayer = 1000;
	else str_to_number(iMaxPlayer, arg3);
	if (iMaxPlayer < 3) iMaxPlayer = 3;

	int iMaxScore = 0;
	if (!*arg4) iMaxScore = KOR_aGuildWarInfo[type].iEndScore;
	else str_to_number(iMaxScore, arg4);
	if (iMaxScore < 5) iMaxScore = 5;

	DWORD dwFlags = 0;
	if (*arg5) str_to_number(dwFlags, arg5);

	int iMapIndex = 0;
	if (*arg6) str_to_number(iMapIndex, arg6);


	g->RequestDeclareWar(opp_g->GetID(), type, iMaxPlayer, iMaxScore, dwFlags, iMapIndex);
#else
	g->RequestDeclareWar(opp_g->GetID(), type);
#endif
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();
#ifdef ENABLE_GENERAL_IN_GUILD
	if (gm_pid != ch->GetPlayerID())
	{
		LPCHARACTER general = CHARACTER_MANAGER::Instance().FindByPID(g->GetGeneralMember());
		if(ch == general)
		{
			// if leader online return general command.
			if(g->GetMasterCharacter() != NULL)
				return;
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
			return;
		}
	}
#else
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전에 대한 권한이 없습니다."));
		return;
	}
#endif

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Aceasta breasla nu exista."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID()
#ifdef __IMPROVED_GUILD_WAR__			
		, 0, 0, 0, 0
#endif
	);
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetMapIndex() == 110)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "You cannot use this in the Arena."));
		return;
	}
#endif	
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));


	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님으로 부터 친구 등록을 거부 당했습니다."), ch->GetName());
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
		else
		{
			CCI* pkCCI = P2P_MANAGER::Instance().Find(arg2);
			if (pkCCI)
			{
				LPDESC pkDesc = pkCCI->pkDesc;
				pkDesc->SetRelay(arg2);
				pkDesc->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s a refuzat cererea de prietenie."), ch->GetName());
				pkDesc->SetRelay("");
			}
		}
#endif		
	}

}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if(ch->GetWear(WEAR_COSTUME_MOUNT))
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;
		
		if (!mountSystem && !mount)
			return;
		if(mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if(mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif
	if (true == ch->UnEquipSpecialRideUniqueItem())
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
		}
	}
	else
	{
		ch->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("인벤토리가 꽉 차서 내릴 수 없습니다."));
	}

}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

#ifdef ENABLE_DECORUM
		if (ch->GetDecorumArena() != NULL)
		{
			ch->GetDecorumArena()->RemoveObserver(ch->GetPlayerID());
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
#endif
		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;
		/*
		   int iSPCost = ch->GetMaxSP() / 3;

		   if (ch->GetSP() < iSPCost)
		   {
		   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"정신력이 부족하여 다른 사람의 장비를 볼 수 없습니다."));
		   return;
		   }
		   ch->PointChange(POINT_SP, -iSPCost);
		 */
		tch->SendEquipment(ch);
	}
}

#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager)
{
	CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
}
#endif

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"이미 파티에 속해 있으므로 가입신청을 할 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주만이 사용 가능한 기능입니다"));
		return;
	}

	//군주 쿨타임 검사
	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_WARP));
		return;
	}

	//군주 몹 소환 비용
	const int WarpPrice = 10000;

	//군주 국고 검사
	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;
	}

	int x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"사용법: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("타제국 유저에게는 이동할수 없습니다"));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"해당 유저는 %d 채널에 있습니다. (현재 채널 %d)"), pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}

			PIXEL_POSITION pos;

			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 에게로 이동합니다"), arg1);
				ch->WarpSet(pos.x, pos.y);

				//군주 돈 삭감
				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				//쿨타임 초기화
				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"타제국 유저에게는 이동할수 없습니다"));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 에게로 이동합니다"), arg1);
	ch->WarpSet(x, y);
	ch->Stop();

	//군주 돈 삭감
	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	//쿨타임 초기화
	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"사용법: transfer <name>"));
		return;
	}

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주만이 사용 가능한 기능입니다"));
		return;
	}

	//군주 쿨타임 검사
	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_TRANSFER));
		return;
	}

	//군주 워프 비용
	const int WarpPrice = 10000;

	//군주 국고 검사
	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, WarpPrice);
		return;
	}


	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 제국 유저는 소환할 수 없습니다."));
				return;
			}
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 님은 %d 채널에 접속 중 입니다. (현재 채널: %d)"), arg1, pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
				return;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s 님을 소환하였습니다."), arg1);

			//군주 돈 삭감
			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
			//쿨타임 초기화
			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"입력하신 이름을 가진 사용자가 없습니다."));
		}

		return;
	}


	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"자신을 소환할 수 없습니다."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 제국 유저는 소환할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 이동할 수 없습니다."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("해당 지역으로 소환할 수 없습니다."));
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

	//군주 돈 삭감
	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
	//쿨타임 초기화
	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"나의 군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"[%s군주] : %s  보유금액 %lld "), EMPIRE_NAME(n), p->name[n], p->money[n]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주 정보"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"[%s군주] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}

}

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}

	// 군주 검사
	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주만이 사용할수 있는 기능입니다"));
		return;
	}

	// 세금설정
	int tax = 0;
	str_to_number(tax,  arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"1-50 사이의 수치를 선택해주세요"));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax);

	// 군주에게 메세지 하나
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"세금이 %d %로 설정되었습니다"));

	// 공지
	char szMsg[1024];

	snprintf(szMsg, sizeof(szMsg), "군주의 명으로 세금이 %d %% 로 변경되었습니다", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "앞으로는 거래 금액의 %d %% 가 국고로 들어가게됩니다.", tax);
	BroadcastNotice(szMsg);

	// 쿨타임 초기화
	ch->SetMC(CHARACTER::MI_TAX);
}

static const DWORD cs_dwMonarchMobVnums[] =
{
	191, //	산견신
	192, //	저신
	193, //	웅신
	194, //	호신
	391, //	미정
	392, //	은정
	393, //	세랑
	394, //	진희
	491, //	맹환
	492, //	보우
	493, //	구패
	494, //	추흔
	591, //	비류단대장
	691, //	웅귀 족장
	791, //	밀교교주
	1304, // 누렁범귀
	1901, // 구미호
	2091, // 여왕거미
	2191, // 거대사막거북
	2206, // 화염왕i
	0,
};

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER	tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주만이 사용할수 있는 기능입니다"));
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

#ifdef ENABLE_MONARCH_MOB_CMD_MAP_CHECK // @warme006
	BYTE pcEmpire = ch->GetEmpire();
	BYTE mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());
	if (mapEmpire != pcEmpire && mapEmpire != 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"자국 영토에서만 사용할 수 있는 기능입니다"));
		return;
	}
#endif

	// 군주 몹 소환 비용
	const int SummonPrice = 5000000;

	// 군주 쿨타임 검사
	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%d 초간 쿨타임이 적용중입니다."), ch->GetMCLTime(CHARACTER::MI_SUMMON));
		return;
	}

	// 군주 국고 검사
	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"국고에 돈이 부족합니다. 현재 : %u 필요금액 : %u"), NationMoney, SummonPrice);
		return;
	}

	const CMob * pkMob;
	DWORD vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	DWORD count;

	// 소환 가능 몹 검사
	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"소환할수 없는 몬스터 입니다. 소환가능한 몬스터는 홈페이지를 참조하세요"));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
			ch->GetMapIndex(),
			ch->GetX() - number(200, 750),
			ch->GetY() - number(200, 750),
			ch->GetX() + number(200, 750),
			ch->GetY() + number(200, 750),
			true,
			pkMob->m_table.bType == CHAR_TYPE_STONE,
			true);

	if (tch)
	{
		// 군주 돈 삭감
		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);

		// 쿨타임 초기화
		ch->SetMC(CHARACTER::MI_SUMMON);
	}
}

static const char* FN_point_string(int apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return LC_TEXT("최대 생명력 +%d");
		case POINT_MAX_SP:	return LC_TEXT("최대 정신력 +%d");
		case POINT_HT:		return LC_TEXT("체력 +%d");
		case POINT_IQ:		return LC_TEXT("지능 +%d");
		case POINT_ST:		return LC_TEXT("근력 +%d");
		case POINT_DX:		return LC_TEXT("민첩 +%d");
		case POINT_ATT_SPEED:	return LC_TEXT("공격속도 +%d");
		case POINT_MOV_SPEED:	return LC_TEXT("이동속도 %d");
		case POINT_CASTING_SPEED:	return LC_TEXT("쿨타임 -%d");
		case POINT_HP_REGEN:	return LC_TEXT("생명력 회복 +%d");
		case POINT_SP_REGEN:	return LC_TEXT("정신력 회복 +%d");
		case POINT_POISON_PCT:	return LC_TEXT("독공격 %d");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:	return LC_TEXT("독공격 %d");
#endif
		case POINT_STUN_PCT:	return LC_TEXT("스턴 +%d");
		case POINT_SLOW_PCT:	return LC_TEXT("슬로우 +%d");
		case POINT_CRITICAL_PCT:	return LC_TEXT("%d%% 확률로 치명타 공격");
		case POINT_RESIST_CRITICAL:	return LC_TEXT("상대의 치명타 확률 %d%% 감소");
		case POINT_PENETRATE_PCT:	return LC_TEXT("%d%% 확률로 관통 공격");
		case POINT_RESIST_PENETRATE: return LC_TEXT("상대의 관통 공격 확률 %d%% 감소");
		case POINT_ATTBONUS_HUMAN:	return LC_TEXT("인간류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return LC_TEXT("동물류 몬스터 타격치 +%d%%");
		case POINT_ATTBONUS_ORC:	return LC_TEXT("웅귀족 타격치 +%d%%");
		case POINT_ATTBONUS_MILGYO:	return LC_TEXT("밀교류 타격치 +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return LC_TEXT("시체류 타격치 +%d%%");
		case POINT_ATTBONUS_DEVIL:	return LC_TEXT("악마류 타격치 +%d%%");
		case POINT_STEAL_HP:		return LC_TEXT("타격치 %d%% 를 생명력으로 흡수");
		case POINT_STEAL_SP:		return LC_TEXT("타력치 %d%% 를 정신력으로 흡수");
		case POINT_MANA_BURN_PCT:	return LC_TEXT("%d%% 확률로 타격시 상대 전신력 소모");
		case POINT_DAMAGE_SP_RECOVER:	return LC_TEXT("%d%% 확률로 피해시 정신력 회복");
		case POINT_BLOCK:			return LC_TEXT("물리타격시 블럭 확률 %d%%");
		case POINT_DODGE:			return LC_TEXT("활 공격 회피 확률 %d%%");
		case POINT_RESIST_SWORD:	return LC_TEXT("한손검 방어 %d%%");
		case POINT_RESIST_TWOHAND:	return LC_TEXT("양손검 방어 %d%%");
		case POINT_RESIST_DAGGER:	return LC_TEXT("두손검 방어 %d%%");
		case POINT_RESIST_BELL:		return LC_TEXT("방울 방어 %d%%");
		case POINT_RESIST_FAN:		return LC_TEXT("부채 방어 %d%%");
		case POINT_RESIST_BOW:		return LC_TEXT("활공격 저항 %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:		return LC_TEXT("두손검 방어 %d%%");
#endif
		case POINT_RESIST_FIRE:		return LC_TEXT("화염 저항 %d%%");
		case POINT_RESIST_ELEC:		return LC_TEXT("전기 저항 %d%%");
		case POINT_RESIST_MAGIC:	return LC_TEXT("마법 저항 %d%%");
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:	return LC_TEXT("마법 저항 %d%%");
#endif
		case POINT_RESIST_WIND:		return LC_TEXT("바람 저항 %d%%");
		case POINT_RESIST_ICE:		return LC_TEXT("냉기 저항 %d%%");
		case POINT_RESIST_EARTH:	return LC_TEXT("대지 저항 %d%%");
		case POINT_RESIST_DARK:		return LC_TEXT("어둠 저항 %d%%");
		case POINT_REFLECT_MELEE:	return LC_TEXT("직접 타격치 반사 확률 : %d%%");
		case POINT_REFLECT_CURSE:	return LC_TEXT("저주 되돌리기 확률 %d%%");
		case POINT_POISON_REDUCE:	return LC_TEXT("독 저항 %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:	return LC_TEXT("독 저항 %d%%");
#endif
#ifdef ENABLE_NEW_TALISMAN_GF
		case POINT_ATTBONUS_ELEC:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_FIRE:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_ICE:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_WIND:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_EARTH:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_DARK:	return LC_TEXT("무당에게 강함 +%d%%");
		
		case POINT_RESIST_HUMAN:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_SWORD_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_TWOHAND_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_DAGGER_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_BELL_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_FAN_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_RESIST_BOW_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_ZODIAC:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_DESERT:	return LC_TEXT("무당에게 강함 +%d%%");
		case POINT_ATTBONUS_INSECT:	return LC_TEXT("무당에게 강함 +%d%%");
		#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW_REDUCTION:	return LC_TEXT("무당에게 강함 +%d%%");
		#endif
#endif
		case POINT_KILL_SP_RECOVER:	return LC_TEXT("%d%% 확률로 적퇴치시 정신력 회복");
		case POINT_EXP_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 경험치 추가 상승");
		case POINT_GOLD_DOUBLE_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 돈 2배 드롭");
		case POINT_ITEM_DROP_BONUS:	return LC_TEXT("%d%% 확률로 적퇴치시 아이템 2배 드롭");
		case POINT_POTION_BONUS:	return LC_TEXT("물약 사용시 %d%% 성능 증가");
		case POINT_KILL_HP_RECOVERY:	return LC_TEXT("%d%% 확률로 적퇴치시 생명력 회복");
//		case POINT_IMMUNE_STUN:	return LC_TEXT("기절하지 않음 %d%%");
//		case POINT_IMMUNE_SLOW:	return LC_TEXT("느려지지 않음 %d%%");
//		case POINT_IMMUNE_FALL:	return LC_TEXT("넘어지지 않음 %d%%");
//		case POINT_SKILL:	return LC_TEXT("");
//		case POINT_BOW_DISTANCE:	return LC_TEXT("");
		case POINT_ATT_GRADE_BONUS:	return LC_TEXT("공격력 +%d");
		case POINT_DEF_GRADE_BONUS:	return LC_TEXT("방어력 +%d");
		case POINT_MAGIC_ATT_GRADE:	return LC_TEXT("마법 공격력 +%d");
		case POINT_MAGIC_DEF_GRADE:	return LC_TEXT("마법 방어력 +%d");
//		case POINT_CURSE_PCT:	return LC_TEXT("");
		case POINT_MAX_STAMINA:	return LC_TEXT("최대 지구력 +%d");
		case POINT_ATTBONUS_WARRIOR:	return LC_TEXT("무사에게 강함 +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return LC_TEXT("자객에게 강함 +%d%%");
		case POINT_ATTBONUS_SURA:		return LC_TEXT("수라에게 강함 +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return LC_TEXT("무당에게 강함 +%d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:	return LC_TEXT("무당에게 강함 +%d%%");
#endif
		case POINT_ATTBONUS_MONSTER:	return LC_TEXT("몬스터에게 강함 +%d%%");
		case POINT_MALL_ATTBONUS:		return LC_TEXT("공격력 +%d%%");
		case POINT_MALL_DEFBONUS:		return LC_TEXT("방어력 +%d%%");
		case POINT_MALL_EXPBONUS:		return LC_TEXT("경험치 %d%%");
		case POINT_MALL_ITEMBONUS:		return LC_TEXT("아이템 드롭율 %.1f배");
		case POINT_MALL_GOLDBONUS:		return LC_TEXT("돈 드롭율 %.1f배");
		case POINT_MAX_HP_PCT:			return LC_TEXT("최대 생명력 +%d%%");
		case POINT_MAX_SP_PCT:			return LC_TEXT("최대 정신력 +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return LC_TEXT("스킬 데미지 %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return LC_TEXT("평타 데미지 %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return LC_TEXT("스킬 데미지 저항 %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return LC_TEXT("평타 데미지 저항 %d%%");
//		case POINT_PC_BANG_EXP_BONUS:	return LC_TEXT("");
//		case POINT_PC_BANG_DROP_BONUS:	return LC_TEXT("");
//		case POINT_EXTRACT_HP_PCT:	return LC_TEXT("");
		case POINT_RESIST_WARRIOR:	return LC_TEXT("무사공격에 %d%% 저항");
		case POINT_RESIST_ASSASSIN:	return LC_TEXT("자객공격에 %d%% 저항");
		case POINT_RESIST_SURA:		return LC_TEXT("수라공격에 %d%% 저항");
		case POINT_RESIST_SHAMAN:	return LC_TEXT("무당공격에 %d%% 저항");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN:	return LC_TEXT("무당공격에 %d%% 저항");
#endif
		default:					return NULL;
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int	year, mon, day;
	int	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_TEXT(" (만료일 : %d년 %d월 %d일)"), year, mon, day);

	return true;
}

ACMD(do_costume)
{
	char buf[512];
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CItem* pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	if (pHair)
	{
		const char* itemName = pHair->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (0 < attr.bType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

	if (pBody)
	{
		const char* itemName = pBody->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (pMount)
	{
		const char* itemName = pMount->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (pAcce)
	{
		const char* itemName = pAcce->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  ACCE: %s", itemName);
		for (int i = 0; i < pAcce->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pAcce->GetAttribute(i);
			if (attr.bType > 0)
			{
				const char * pAttrName = FN_point_string(attr.bType);
				if (pAttrName == NULL)
					continue;

				snprintf(buf, sizeof(buf), FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}
		if (pAcce->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAcce);
	}
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pWeapon)
	{
		const char* itemName = pWeapon->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  WEAPON : %s", itemName);

		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

ACMD(do_inventory)
{
	int	index = 0;
	int	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s", index, item ? item->GetName(ch->GetLanguage()) : "<NONE>");
		++index;
	}
}

//gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}

#ifdef NEW_PET_SYSTEM
ACMD(do_CubePetAdd) {

	int pos = 0;
	int invpos = 0;

	const char *line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
		return;
	const std::string& strArg1 = std::string(arg1);
	switch (LOWER(arg1[0]))
	{
	case 'a':	// add cue_index inven_index
	{
		if (0 == arg2[0] || !isdigit(*arg2) ||
			0 == arg3[0] || !isdigit(*arg3))
			return;

		str_to_number(pos, arg2);
		str_to_number(invpos, arg3);

	}
	break;

	default:
		return;
	}

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->SetItemCube(pos, invpos);
	else
		return;

}

ACMD(do_PetSkill) {
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	DWORD skillslot = 0;
	str_to_number(skillslot, arg1);
	if (skillslot > 2 || skillslot < 0)
		return;

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->DoPetSkill(skillslot);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi aver evocato il pet per procedere");
}

ACMD(do_FeedCubePet) {
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	DWORD feedtype = 0;
	str_to_number(feedtype, arg1);
	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->ItemCubeFeed(feedtype);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Non hai un pet evocato.");
}

ACMD(do_PetEvo) {

	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen()) {
		ch->ChatPacket(CHAT_TYPE_INFO, "[Evoluzione] Non puoi evolvere il tuo pet ora.");
		return;
	}
	if (ch->GetNewPetSystem()->IsActivePet()) {

		int it[3][1] = { 
						{ 28524 }, //Here Modify Items to request for 1 evo
						{ 28525 }, //Here Modify Items to request for 2 evo
						{ 28526 }  //Here Modify Items to request for 3 evo
		};
		int ic[3][1] = {{ 1 },
						{ 2 },
						{ 3 }
		};
		int tmpevo = ch->GetNewPetSystem()->GetEvolution();

		if (ch->GetNewPetSystem()->GetLevel() == 40 && tmpevo == 0 ||
			ch->GetNewPetSystem()->GetLevel() == 81 && tmpevo == 1 ||
			ch->GetNewPetSystem()->GetLevel() == 81 && tmpevo == 2) {
			for (int b = 0; b < 1; b++) {
				if (ch->CountSpecifyItem(it[tmpevo][b]) < ic[tmpevo][b]) {
					ch->ChatPacket(CHAT_TYPE_INFO, "[Evoluzione] Item richiesti:");
					for (int c = 0; c < 1; c++) {
						DWORD vnum = it[tmpevo][c];
						ch->ChatPacket(CHAT_TYPE_INFO, "%s X%d", ITEM_MANAGER::instance().GetTable(vnum)->szLocaleName , ic[tmpevo][c]);
					}
					return;
				}
			}
			for (int c = 0; c < 1; c++) {
				ch->RemoveSpecifyItem(it[tmpevo][c], ic[tmpevo][c]);
			}
			ch->GetNewPetSystem()->IncreasePetEvolution();

		}
		else {
			ch->ChatPacket(CHAT_TYPE_INFO, "Non puoi ancora evolvere il tuo pet!");
			return;
		}

	}else
		ch->ChatPacket(CHAT_TYPE_INFO, "Il tuo pet deve essere evocato.");

}

#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
ACMD(do_cube)
{

	const char *line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		return;
	}

	switch (LOWER(arg1[0]))
	{
	case 'o':	// open
		Cube_open(ch);
		break;

	default:
		return;
	}
}
#else
ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	const char *line;

	int inven_type = 0;

	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		// print usage
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	// r_info (request information)
	// /cube r_info     ==> (Client -> Server) 현재 NPC가 만들 수 있는 레시피 요청
	//					    (Server -> Client) /cube r_list npcVNUM resultCOUNT 123,1/125,1/128,1/130,5
	//
	// /cube r_info 3   ==> (Client -> Server) 현재 NPC가 만들수 있는 레시피 중 3번째 아이템을 만드는 데 필요한 정보를 요청
	// /cube r_info 3 5 ==> (Client -> Server) 현재 NPC가 만들수 있는 레시피 중 3번째 아이템부터 이후 5개의 아이템을 만드는 데 필요한 재료 정보를 요청
	//					   (Server -> Client) /cube m_info startIndex count 125,1|126,2|127,2|123,5&555,5&555,4/120000@125,1|126,2|127,2|123,5&555,5&555,4/120000
	//
	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
	case 'o':	// open
		Cube_open(ch);
		break;

	case 'c':	// close
		Cube_close(ch);
		break;

	case 'l':	// list
		Cube_show_list(ch);
		break;

	case 'a':	// add cue_index inven_index
	{
		if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
			return;

		str_to_number(cube_index, arg2);
		str_to_number(inven_index, arg3);
		Cube_add_item(ch, cube_index, inven_index);
	}
	break;

	case 'd':	// delete
	{
		if (0 == arg2[0] || !isdigit(*arg2))
			return;

		str_to_number(cube_index, arg2);
		Cube_delete_item(ch, cube_index);
	}
	break;

	case 'm':	// make
		if (0 != arg2[0])
		{
			while (true == Cube_make(ch))
				dev_log(LOG_DEB0, "cube make success");
		}
		else
			Cube_make(ch);
		break;

	default:
		return;
	}
}
#endif
/*
ACMD(do_in_game_mall)
{
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
			case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
			case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
			case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
			case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
			case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
			case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
			case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
			default:
				if (test_server == true)
				{
					country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0';
				}
				break;
		}

		char buf[512+1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *) buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i) {
			sas[i+i] = hex[digest[i] >> 4];
			sas[i+i+1] = hex[digest[i] & 0x0f];
		}
		sas[i+i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
				g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}
*/
ACMD(do_in_game_mall)
{
	char buf[512+1];
	char sas[33];
	MD5_CTX ctx;
	const char sas_key[] = "GF9001";
	
	char language[3];
	strcpy(language, "it");//If you have multilanguage, update this
	
	snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char *) buf, strlen(buf));
#ifdef __FreeBSD__
	MD5End(&ctx, sas);
#else
	static const char hex[] = "0123456789abcdef";
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	int i;
	for (i = 0; i < 16; ++i) {
		sas[i+i] = hex[digest[i] >> 4];
		sas[i+i+1] = hex[digest[i] & 0x0f];
	}
	sas[i+i] = '\0';
#endif

	snprintf(buf, sizeof(buf), "mall https://zayos.eu/shop?pid=%u&lang=%s&sid=%d&sas=%s",
			ch->GetPlayerID(), language, g_server_id, sas);

	ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
}
// 주사위
ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_DICE_INFO, LC_TEXT("%s님이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_DICE_INFO, LC_TEXT("당신이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), n, start, end);
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%s님이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"당신이 주사위를 굴려 %d가 나왔습니다. (%d-%d)"), n, start, end);
#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi aprire il magazzino in un dungeon o in guerra."));
		return;
	}

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}
ACMD(do_force_logout)
{
	LPDESC pDesc=DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

#ifdef OFFLINE_SHOP
ACMD(do_open_offline_shop)
{
	if (ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asigurati-va ca nu aveti alte ferestre deschise."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu puteti face acest lucru, pe aceasta harta."));
	return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe acest canal."));
		return;
	}

	COfflineShopManager::instance().ResetOfflineShopStatus(ch);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenOfflineShop");
}

ACMD(do_withdraw_offline_shop_money)
{
	if (ch->IsDead() || ch->GetExchange() || ch->GetMyShop() || ch->GetOfflineShop() || ch->IsCubeOpen() || ch->IsOpenSafebox() || ch->GetShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asigurati-va ca nu aveti alte ferestre deschise."));
		return;
	}

	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asteapta un moment."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe aceasta harta."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe acest canal."));
		return;
	}

	ch->SetMyOfflineShopTime();

	COfflineShopManager::instance().WithdrawAllMoney(ch);
}

ACMD(do_withdraw_offline_shop_cheque)
{
	if (ch->IsDead() || ch->GetExchange() || ch->GetMyShop() || ch->GetOfflineShop() || ch->IsCubeOpen() || ch->IsOpenSafebox() || ch->GetShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asigurati-va ca nu aveti ferestre deschise."));
		return;
	}

	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asteapta un moment."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe aceasta harta."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe acest canal."));
		return;
	}

	ch->SetMyOfflineShopTime();

	COfflineShopManager::instance().WithdrawAllCheque(ch);
}

ACMD(do_retrieve_offline_shop_item)
{
	if (ch->IsDead() || ch->GetExchange() || ch->GetMyShop() || ch->GetOfflineShop() || ch->IsCubeOpen() || ch->IsOpenSafebox() || ch->GetShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asigurati-va ca nu aveti ferestre deschise."));
		return;
	}

	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Asteapta un moment."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe aceasta harta."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Magazin Offline> Nu poti face asta pe acest canal."));
		return;
	}

	if (ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Trebuie sa deschideti magazinul inainte sa faceti acest lucru."));
		return;
	}

	ch->SetMyOfflineShopTime();

	COfflineShopManager::instance().FetchMyItems(ch);
}
#ifdef _OPEN_SEARCH_WINDOW_
// ACMD(do_open_search_window)
// {
	// ch->ChatPacket(CHAT_TYPE_COMMAND, "umut_k_cof");
// }
// ACMD(do_cofres_umut)
// {
	// ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenShopSearch 1");
// }
// ACMD(do_log_ekran)
// {
	// ch->ChatPacket(CHAT_TYPE_COMMAND, "umutk_log");
// }
#endif
ACMD(do_destroy_offlineshop)
{
	// 
	if (!ch->IsGM())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);

	LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(vid);

	if (!npc)
	{
		sys_err("do_destroy_offlineshop can't find the npc vid: %d", vid);
		return;
	}

	if (!npc->IsOfflineShopNPC())
	{
		sys_err("do_destroy_offlineshop: the VID isn't OfflineShopNPC: %d", vid);
		return;
	}

	COfflineShopManager::instance().DestroyOfflineShop(NULL, vid, false);
	M2_DESTROY_CHARACTER(npc);
	
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
ACMD(do_ride)
{
	if (ch->IsDead() || ch->IsStun())
		return;

	// if (ch->GetMapIndex() == 113 || ch->GetMapIndex() == 1 || ch->GetMapIndex() == 21 || ch->GetMapIndex() == 41 || ch->GetMapIndex() == 74 || ch->GetMapIndex() == 83 || ch->GetMapIndex() == 106 || ch->GetMapIndex() == 102 || ch->GetMapIndex() == 114)
		// return;
#ifdef ENABLE_DECORUM
	if (CDecoredArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
		return;
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->IsPolymorphed() == true){
		ch->ChatPacket(CHAT_TYPE_INFO, "?v?tozva nem teheted ezt.");
		return;
	}

	if(ch->GetWear(WEAR_COSTUME_MOUNT))
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;
		
		if (!mountSystem && !mount) {
			return;
		}

		if(mount->GetValue(1) != 0) {
			mobVnum = mount->GetValue(1);
		}

		if (ch->GetMountVnum())
		{
			if(mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		else
		{
			if(mountSystem->CountSummoned() == 1)
			{
				mountSystem->Mount(mobVnum, mount);
			}
		}
		
		return;
	}
#endif

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
		return;
	}

	if (ch->GetMountVnum())
	{
		do_unmount(ch, NULL, 0, 0);
		return;
	}

	if (ch->GetHorse() != NULL)
	{
		ch->StartRiding();
		return;
	}

	for (UINT i=0; i<INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (NULL == item)
			continue;

		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT)	{
			ch->UseItem(TItemPos (INVENTORY, i));
			return;
		}
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Intai trebuie sa chemi calul."));
}
#else
ACMD(do_ride)
{
    dev_log(LOG_DEB0, "[DO_RIDE] start");
    if (ch->IsDead() || ch->IsStun())
	return;

    // 내리기
    {
	if (ch->IsHorseRiding())
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] stop riding");
	    ch->StopRiding();
	    return;
	}

	if (ch->GetMountVnum())
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] unmount");
	    do_unmount(ch, NULL, 0, 0);
	    return;
	}
    }
#ifdef ENABLE_DECORUM
	if (CDecoredArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
		return;
#endif
    // 타기
    {
	if (ch->GetHorse() != NULL)
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] start riding");
	    ch->StartRiding();
	    return;
	}

	for (BYTE i=0; i<INVENTORY_MAX_NUM; ++i)
	{
	    LPITEM item = ch->GetInventoryItem(i);
	    if (NULL == item)
		continue;

	    // 유니크 탈것 아이템
		if (item->IsRideItem())
		{
			if (
				NULL==ch->GetWear(WEAR_UNIQUE1)
				|| NULL==ch->GetWear(WEAR_UNIQUE2)
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				|| NULL==ch->GetWear(WEAR_COSTUME_MOUNT)
#endif
			)
			{
				dev_log(LOG_DEB0, "[DO_RIDE] USE UNIQUE ITEM");
				//ch->EquipItem(item);
				ch->UseItem(TItemPos (INVENTORY, i));
				return;
			}
		}

	    // 일반 탈것 아이템
	    // TODO : 탈것용 SubType 추가
	    switch (item->GetVnum())
	    {
		case 71114:	// 저신이용권
		case 71116:	// 산견신이용권
		case 71118:	// 투지범이용권
		case 71120:	// 사자왕이용권
		    dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
		    ch->UseItem(TItemPos (INVENTORY, i));
		    return;
	    }

		// GF mantis #113524, 52001~52090 번 탈것
		if( (item->GetVnum() > 52000) && (item->GetVnum() < 52091) )	{
			dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
			ch->UseItem(TItemPos (INVENTORY, i));
		    return;
		}
	}
    }


    // 타거나 내릴 수 없을때
    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Devi prima evocare una cavalcatura."));
}
#endif

#ifdef __AUCTION__
// temp_auction
ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_MAX_NUM; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "name : %s id : %d", item->GetProto()->szName, item->GetID());
	}
}

// temp_auction

ACMD(do_enroll_auction)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	char arg4[256];
	two_arguments (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	DWORD item_id = strtoul(arg1, NULL, 10);
	BYTE empire = strtoul(arg2, NULL, 10);
	int bidPrice = strtol(arg3, NULL, 10);
	int immidiatePurchasePrice = strtol(arg4, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_auction(ch, item, empire, bidPrice, immidiatePurchasePrice);
}

ACMD(do_enroll_wish)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	DWORD item_num = strtoul(arg1, NULL, 10);
	BYTE empire = strtoul(arg2, NULL, 10);
	int wishPrice = strtol(arg3, NULL, 10);

	AuctionManager::instance().enroll_wish(ch, item_num, empire, wishPrice);
}

ACMD(do_enroll_sale)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	DWORD item_id = strtoul(arg1, NULL, 10);
	DWORD wisher_id = strtoul(arg2, NULL, 10);
	int salePrice = strtol(arg3, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_sale(ch, item, wisher_id, salePrice);
}

// temp_auction
// packet으로 통신하게 하고, 이건 삭제해야한다.
ACMD(do_get_auction_list)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	two_arguments (one_argument (argument, arg1, sizeof(arg1)), arg2, sizeof(arg2), arg3, sizeof(arg3));

	AuctionManager::instance().get_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10), strtoul(arg3, NULL, 10));
}
//
//ACMD(do_get_wish_list)
//{
//	char arg1[256];
//	char arg2[256];
//	char arg3[256];
//	two_arguments (one_argument (argument, arg1, sizeof(arg1)), arg2, sizeof(arg2), arg3, sizeof(arg3));
//
//	AuctionManager::instance().get_wish_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10), strtoul(arg3, NULL, 10));
//}
ACMD (do_get_my_auction_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_get_my_purchase_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_purchase_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_bid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_impur)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().immediate_purchase (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_get_auctioned_item)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_buy_sold_item)
{
	char arg1[256];
	char arg2[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_cancel_auction)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_auction (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_wish)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_wish (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_sale)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_sale (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_rebid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().rebid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_bid_cancel)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid_cancel (ch, strtoul(arg1, NULL, 10));
}
#endif

#ifdef __BATTLE_PASS__
ACMD(open_battlepass)
{
	if (ch->v_counts.empty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi aver attivato il Pass Battaglia per usufruire di questa funzione.");
		return;
	}

	if (ch->missions_bp.empty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi aver attivato il Pass Battaglia per usufruire di questa funzione.");
		return;
	}

	if (ch->IsOpenSafebox() || ch->IsDead() || ch->GetExchange() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Nu poti face asta!");
		return;
	}

	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);

	int month = vKey.tm_mon;
	
	if (month != ch->iMonthBattlePass)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Time Expire!");
		return;
	}

	for (int i=0; i<ch->missions_bp.size(); ++i)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "missions_bp %d %d %d %d", i, ch->missions_bp[i].type, ch->missions_bp[i].vnum, ch->missions_bp[i].count);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "info_missions_bp %d %d %d %s %s", i, ch->v_counts[i].count, ch->v_counts[i].status, ch->rewards_bp[i].name, ch->rewards_bp[i].image);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "rewards_missions_bp %d %d %d %d %d %d %d", i, ch->rewards_bp[i].vnum1, ch->rewards_bp[i].vnum2, ch->rewards_bp[i].vnum3, ch->rewards_bp[i].count1, ch->rewards_bp[i].count2 ,ch->rewards_bp[i].count3);
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "size_missions_bp %d ", ch->missions_bp.size());
	ch->ChatPacket(CHAT_TYPE_COMMAND, "final_reward %d %d %d %d %d %d", ch->final_rewards[0].f_vnum1, ch->final_rewards[0].f_vnum2, ch->final_rewards[0].f_vnum3, ch->final_rewards[0].f_count1, ch->final_rewards[0].f_count2 ,ch->final_rewards[0].f_count3);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "show_battlepass");
}

ACMD(final_reward_battlepass)
{
	if (ch->v_counts.empty())
		return;

	if (ch->missions_bp.empty())
		return;

	if (ch->v_counts[0].status == 2)
		return;

	if (ch->IsOpenSafebox() || ch->IsDead() || ch->GetExchange() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't do that.!");
		return;
	}

	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);

	int month = vKey.tm_mon;
	
	if (month != ch->iMonthBattlePass)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Time Expired!");
		return;
	}

	for (int i=0; i<ch->missions_bp.size(); ++i)
	{
		if (ch->missions_bp[i].count != ch->v_counts[i].count)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You haven't completed all the missions!");
			return;
		}
	}

	ch->FinalRewardBattlePass();
}
#endif

#ifdef __SPECIAL_STORAGE_SYSTEM__
ACMD(do_sort_special_storage)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	if(!ch->CanHandleItem()||ch->IsDead())
		return;

	if (ch->IsHack())
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (ch->IsOwnerEditShopOffline() || ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
		return;
	// PREVENT BUG WHILE EDIT MODE

	BYTE window_type;

	str_to_number(window_type, arg1);

	if (window_type >= SKILLBOOK_INVENTORY && window_type <= GENERAL_INVENTORY)
		ch->SortSpecialStorage(window_type, atoi(arg2) ? true : false);
}
ACMD(do_storage_split_item)
{
	const char * line;

	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	one_argument(line, arg3, sizeof(arg3));

	if(!ch->CanHandleItem()||ch->IsDead())
		return;
	
	if (ch->IsHack())
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (ch->IsOwnerEditShopOffline() || ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
		return;
	// PREVENT BUG WHILE EDIT MODE

	if (*arg1 && *arg2 && *arg3)
	{
		int slot = atoi(arg1);
		int count = atoi(arg2);

		BYTE window_type = atoi(arg3);

		if (ch->IsObserverMode())
			return;

		if (ch->IsDead() || ch->IsStun())
			return;

		if (slot >= 0 && slot <= SPECIAL_STORAGE_INVENTORY_MAX_NUM && count >= 0 && count <= g_bItemCountLimit)
		{
			LPITEM item = ch->GetSpecialStorageItem(slot, window_type);

			if (item && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
			{
				int itemCount = item->GetCount();

				TItemPos ItemCell;

				ItemCell.window_type = window_type;
				ItemCell.cell = item->GetCell();

				for (int i = 0; i < (itemCount / count) - 1; ++i)
				{
					int iSlot = ch->GetEmptySpecialStorageSlot(item);

					TItemPos DestItemCell;

					DestItemCell.window_type = window_type;
					DestItemCell.cell = iSlot;

					ch->MoveItem(ItemCell, DestItemCell, count);
				}
			}
		}
	}
}

ACMD(do_qitem)
{
	if(!ch->CanHandleItem()||ch->IsDead())
		return;
	std::vector<std::string> vecArgs;
	split_argument(argument,vecArgs);
	if (vecArgs.size() < 4){return;}
	BYTE type = 0, invType = 0;
	WORD invPos=0;
	str_to_number(type,vecArgs[1].c_str());
	str_to_number(invType,vecArgs[2].c_str());
	str_to_number(invPos,vecArgs[3].c_str());
	LPITEM item=NULL;
	if (!(item = ch->GetItem(TItemPos(invType,invPos))))
		return;
	DWORD itemVnum = item->GetVnum();

	// New Pet System
	if (ch->GetNewPetSystem() && ch->GetNewPetSystem()->IsActivePet() && item->GetVnum() >= 55701 && item->GetVnum() <= 55710)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi mandare via il tuo pet!");
		return;
	}
	// Support System
/* 	else if (ch->GetSupportSystem() && ch->GetSupportSystem()->IsActiveSupport() && (item->GetVnum() >= 61020 && item->GetVnum() <= 61024))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You need deactive your support shaman!");
		return;
	}
 */
	if(type == 1) // destroy
	{
		if (item->IsExchanging() || item->isLocked() || item->IsEquipped())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Non puoi distruggere questo oggetto!");
			return;
		}
		ch->ChatPacket(CHAT_TYPE_INFO, "%s distrutto con successo.",item->GetName());
		item->RemoveFromCharacter();
		item->Save();
	}
	else if(type == 2) // sell
	{
		if (item->IsExchanging() || item->isLocked() || item->IsEquipped())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Non puoi vendere questo oggetto.");
			return;
		}
		DWORD dwPrice = item->GetShopBuyPrice()*item->GetCount();
		if (1999999999 <= ch->GetGold()+dwPrice)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Converti degli Yang a Won prima.");
			return;
		}
		ch->ChatPacket(CHAT_TYPE_INFO, "%s venduto con successo.",item->GetName());
		item->RemoveFromCharacter();
		item->Save();
		ch->PointChange(POINT_GOLD, dwPrice);
	}
}

ACMD(do_transfer_inv_storage)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsHack())
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (ch->IsOwnerEditShopOffline() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
		return;
	// PREVENT BUG WHILE EDIT MODE

	DWORD dwPos = atoi(arg1);

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM)
	{
		LPITEM item = ch->GetInventoryItem(dwPos);

		if (item && item->IsSpecialStorageItem())
		{
			int iEmptyPos = ch->GetEmptySpecialStorageSlot(item);
			
			if (iEmptyPos != -1)
				ch->MoveItem(TItemPos(INVENTORY, item->GetCell()), TItemPos(item->GetSpecialWindowType(), iEmptyPos), item->GetCount());
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "Non hai abbastanza spazio nell'inventario speciale.");
		}
	}
}

ACMD(do_transfer_storage_inv)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	DWORD dwPos = atoi(arg1);

	BYTE window_type = atoi(arg2);

	// PREVENT BUG WHILE EDIT MODE
	if (ch->IsOwnerEditShopOffline() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
		return;
	// PREVENT BUG WHILE EDIT MODE

	if (ch->IsHack())
		return;

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM && window_type >= SKILLBOOK_INVENTORY && window_type <= GENERAL_INVENTORY)
	{
		LPITEM item = ch->GetSpecialStorageItem(dwPos, window_type);

		if (item && item->IsSpecialStorageItem())
		{
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());
			
			if (iEmptyPos != -1)
				ch->MoveItem(TItemPos(item->GetSpecialWindowType(), item->GetCell()), TItemPos(INVENTORY, iEmptyPos), item->GetCount());
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"소지하고 있는 아이템이 너무 많습니다."));
		}
	}
}
#endif

#ifdef ENABLE_AURA_SYSTEM
#ifdef CLEAR_BONUS_AURA
ACMD(do_clear_bonus_aura)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsOwnerEditShopOffline() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop()){
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Chiudi prima tutte le finestre."));
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	if (dwVnum < 0 || dwVnum >= INVENTORY_MAX_NUM)
		return;

	LPITEM item = ch->GetInventoryItem(dwVnum);

	if (!item || item->IsExchanging() || item->IsEquipped())
		return;

	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_AURA){
		if (ch->CountSpecifyItem(49007) >= 1){
			for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++){
				item->SetForceAttribute(j, 0, 0);
			}

			ch->RemoveSpecifyItem(49007, 1);
			item->Save();
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"The aura bonuses are cleared succesfully."));
		}
		else{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"You don't have item for clear aura bonuses."));
			return;
		}
	}
	else{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"You can use this item only on auras."));
		return;
	}
}
#endif
static int GetAbsorb(int level)
{
	if (level >= 1 && level <= 100)
		return 1;
	else if (level >= 201 && level <= 299)
		return 2;
	else if (level >= 300 && level <= 399)
		return 3;
	else if (level >= 400 && level <= 499)
		return 4;
	else if (level == 500)
		return 5;
	return 1;
}


static void UpAttr(LPITEM aura)
{
	if (!aura)
		return;
	//int value = GetAbsorb(aura->GetSocket(1));
	int value = aura->GetSocket(1);
	for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
	{
		if (aura->GetAttributeType(j)>0)
		{
			if ( ((aura->GetAttributeValue(j) * value)/1000) < 1 )
				aura->SetForceAttribute(j, aura->GetAttributeType(j), 1);
			else
				aura->SetForceAttribute(j, aura->GetAttributeType(j), ((aura->GetAttributeValue(j) * value)/1000));
		}
	}
}

static void AuraRefine(LPCHARACTER ch, int aura_pos, int item_pos) // ok i think this is for level maybe idk 
{
	if (!ch || aura_pos == -1 || item_pos == -1)
		return;
	if (aura_pos >= 180 || item_pos >= 180) // equip
		return;
	LPITEM aura = ch->GetInventoryItem(aura_pos);
	LPITEM material = ch->GetInventoryItem(item_pos);
	if (!aura || !material)
		return;
	if (!ch->GetAuraRefine()) // gui not open <>
		return;
	if (ch->IsDead()
		|| ch->IsStun()
		//|| ch->IsAcceOpen()
		|| ch->GetExchange()
		|| ch->GetShopOwner()
		|| ch->IsOpenSafebox()
		|| ch->GetShop()
		//|| ch->IsAttrTransferOpen()
		|| ch->IsCubeOpen()
		|| ch->GetOfflineShop()
		|| ch->GetOfflineShopOwner()
		|| ch->isChangeLookOpened()
		|| ch->IsObserverMode()
		//|| ch->IsOpenMailBox()
		|| ch->IsWarping()
		|| ch->GetAuraAbs())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AURA_HAVE_BLOCK"));
		return;
	}
	if (ch->GetGold() < 2000000)
		return;
	bool bFlag = false;
	if (aura->GetType() == ITEM_COSTUME && aura->GetSubType() == COSTUME_AURA)
	{
		long sockets[ITEM_SOCKET_MAX_NUM];
		thecore_memcpy(sockets, aura->GetSockets(), sizeof(sockets));
		if (sockets[1] >= 500) // max level
			return;
		if (sockets[2] != 100) { return; } // exp
		int max_level[] = { 9, 49, 99, 249, 499 };
		for (BYTE j = 0; j < 5; ++j)
		{
			if (sockets[1] == max_level[j] && material->GetVnum() == 49991 + j)
			{
				material->SetCount(material->GetCount() - 1);
				LPITEM n_aura = ITEM_MANAGER::instance().CreateItem(49002 + j, 1);
				if (!n_aura)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AURA_CONTANT_GM"));
					sys_err("Can't create new aura player name %s give this material for player vnum=%d ", ch->GetName(), 49991 + j);
					return;
				}
				n_aura->SetSocket(0, sockets[0] + 1);
				n_aura->SetSocket(1, sockets[1] + 1);
				n_aura->SetSocket(2, 0);
				if (aura->GetAttributeType(0)>0)
				{
					n_aura->SetAttributes(aura->GetAttributes());
					UpAttr(n_aura);
				}
				aura->RemoveFromCharacter();// del
				n_aura->AddToCharacter(ch, TItemPos(INVENTORY, aura_pos));
				n_aura->Save();
				ITEM_MANAGER::instance().FlushDelayedSave(n_aura);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "AuraMessage 1");
				bFlag = true;
				break;
			}
		}
		if (bFlag == false)
		{
			if (material->GetVnum() == 49990)
			{
				material->SetCount(material->GetCount() - 1);
				aura->SetSocket(0, sockets[1] + 1); //level <>
				aura->SetSocket(1, sockets[1] + 1); //sub level <>
				aura->SetSocket(2, 0);
				//UpAttr(aura);
				aura->Save();
				ch->ChatPacket(CHAT_TYPE_COMMAND, "AuraMessage 2");
			}
		}
		//ch->ChangeGold(-25000000); for my server files
		ch->PointChange(POINT_GOLD, -2000000);
	}
}

static void AuraUpgradeExp(LPCHARACTER ch, int aura_pos, int item_pos)
{
	if (!ch || aura_pos == -1 || item_pos == -1)
		return;
	if (aura_pos >= 180 || item_pos >= 180) // equip
		return;
	LPITEM aura = ch->GetInventoryItem(aura_pos);
	LPITEM material = ch->GetInventoryItem(item_pos);
	if (!aura || !material)
		return;
	// if (!ch->GetAuraRefine()) // gui not open <>
		// return;
	if (ch->IsDead()
		|| ch->IsStun()
		//|| ch->IsAcceOpen()
		|| ch->GetExchange()
		|| ch->GetShopOwner()
		|| ch->IsOpenSafebox()
		|| ch->GetShop()
		//|| ch->IsAttrTransferOpen()
		|| ch->IsCubeOpen()
		|| ch->GetOfflineShop()
		|| ch->GetOfflineShopOwner()
		|| ch->isChangeLookOpened()
		|| ch->IsObserverMode()
		//|| ch->IsOpenMailBox()
		|| ch->IsWarping()
		|| ch->GetAuraRefine()
		|| ch->GetAuraAbs())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AURA_HAVE_BLOCK"));
		return;
	}

	bool bFlag = false;
	if (aura->GetType() == ITEM_COSTUME && aura->GetSubType() == COSTUME_AURA)
	{
		long sockets[ITEM_SOCKET_MAX_NUM];
		thecore_memcpy(sockets, aura->GetSockets(), sizeof(sockets));
		if (sockets[1] >= 500) // max level
			return;
		if (sockets[2] == 100) { return; } // exp it s max

		DWORD dwExpVnum = material->GetVnum();
		DWORD dwCount = material->GetCount();
		
		// vnums for these improvements
		if (!(dwExpVnum >= 49990 && dwExpVnum <= 49995))
			return;
		
		for (int i = 0; i < dwCount; ++i)
		{
			material = ch->GetInventoryItem(item_pos);
			
			if (material == NULL)
			{
				// ch->ChatPacket(CHAT_TYPE_INFO, "not found really this item.");
				break;
			}
			
			// start check if is material again
			dwExpVnum = material->GetVnum();

			if (material->GetCount() <= 0)
			{
				// ch->ChatPacket(CHAT_TYPE_INFO, "count equal or lower than 0");
				break;
			}
			
			if (!(dwExpVnum >= 49990 && dwExpVnum <= 49995))
				return;			
			
			long lValSocket = aura->GetSocket(2);
			
			if (lValSocket >= 100) // if is already in this loop 100% exp, stop and break
			{
				// ch->ChatPacket(CHAT_TYPE_INFO, "1good, its 100%");
				break;
			}
			
			long lValueGet = material->GetValue(2);
			
			if (lValueGet + lValSocket >= 100)
			{
				aura->SetSocket(2, 100);
				// ch->ChatPacket(CHAT_TYPE_INFO, "good, its 100%");
				break;
			}
			else
				aura->SetSocket(2, lValueGet + lValSocket);
			
			material->SetCount(material->GetCount() - 1);
			
			// ch->ChatPacket(CHAT_TYPE_INFO, "i: %d and current count: %d", i, dwCount);
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "Successo, hai aumentato l'EXP, attuale :%d", aura->GetSocket(2));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "AuraMessage 111");
	}
}


static void AuraAbsorb(LPCHARACTER ch, int aura_pos, int item_pos)
{
	if (!ch || aura_pos == -1 || item_pos == -1)
		return;
	if (aura_pos >= 180 || item_pos >= 180) // equip
		return;
	LPITEM aura = ch->GetInventoryItem(aura_pos);
	LPITEM item = ch->GetInventoryItem(item_pos);
	if (!aura || !item)
		return;
	if (!ch->GetAuraAbs()) // gui not open <>
		return;
	if (aura->GetAttributeType(0)>0)
		return;
	if (ch->GetGold() < 2000000)
		return;
	if (ch->IsDead()
		|| ch->IsStun()
		//|| ch->IsAcceOpen()
		|| ch->GetExchange()
		|| ch->GetShopOwner()
		|| ch->IsOpenSafebox()
		|| ch->GetShop()
		//|| ch->IsAttrTransferOpen()
		|| ch->IsCubeOpen()
		|| ch->GetOfflineShop()
		|| ch->GetOfflineShopOwner()
		|| ch->isChangeLookOpened()
		|| ch->IsObserverMode()
		//|| ch->IsOpenMailBox()
		|| ch->IsWarping()
		|| ch->GetAuraRefine())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("AURA_HAVE_BLOCK"));
		return;
	}

	if (aura->GetType() == ITEM_COSTUME && aura->GetSubType() == COSTUME_AURA && item->GetType() == ITEM_ARMOR &&
		(item->GetSubType() == ARMOR_EAR || item->GetSubType() == ARMOR_TALISMAN ||
		item->GetSubType() == ARMOR_FOOTS || item->GetSubType() == ARMOR_HEAD ||
		item->GetSubType() == ARMOR_NECK || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_WRIST))
	{
		int cur = 0;
		for (BYTE j = 0; j<3; ++j)
		{
			if (item->GetNewAttributeType(j)>0) // proto xdd
			{
				aura->SetForceAttribute(cur++, item->GetNewAttributeType(j), item->GetNewAttributeValue(j));
			}
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			if (item->GetAttributeType(j)>0 && cur < ITEM_ATTRIBUTE_MAX_NUM)
			{
				aura->SetForceAttribute(cur++, item->GetAttributeType(j), item->GetAttributeValue(j));
			}
		}


		
		UpAttr(aura);
		M2_DESTROY_ITEM(item->RemoveFromCharacter());
		aura->Save();
		//ch->ChangeGold(-25000000); 
		ch->PointChange(POINT_GOLD, -2000000);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "AuraMessage 3");
	}
}

ACMD(do_aura)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) // cmd 1 <>
	{
		return;
	}
	int id = 0;
	str_to_number(id, vecArgs[1].c_str());
	switch (id)
	{
		// Refine <>
	case 1:
	{
		ch->SetAuraRefine(false); // close gui
	}
	break;
	case 2:
	{
		if (vecArgs.size() < 4) // cmd 1 <>
		{
			return;
		}
		int pos_0, pos_1 = -1;
		str_to_number(pos_0, vecArgs[2].c_str());
		str_to_number(pos_1, vecArgs[3].c_str());
		AuraRefine(ch, pos_0, pos_1);
	}
	break;

	// Absorb
	case 3:
	{
		ch->SetAuraAbs(false); // close gui
	}
	break;

	case 4:
	{
		if (vecArgs.size() < 4) // cmd 1 <>
		{
			return;
		}
		int pos_0, pos_1 = -1;
		str_to_number(pos_0, vecArgs[2].c_str());
		str_to_number(pos_1, vecArgs[3].c_str());
		AuraAbsorb(ch, pos_0, pos_1);
	}
	break;
	case 5:
	{
		if (vecArgs.size() < 4) // cmd 1 <>
		{
			return;
		}
		int pos_0, pos_1 = -1;
		str_to_number(pos_0, vecArgs[2].c_str());
		str_to_number(pos_1, vecArgs[3].c_str());
		AuraUpgradeExp(ch, pos_0, pos_1);
	}
	break;

	}
}
#endif

ACMD(do_hizlistatuver)
{
    char arg1[256];
    one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1)
        return;

    if (ch->IsPolymorphed())
    {
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Successo."));
        return;
    }

    if (ch->GetPoint(POINT_STAT) <= 0)
        return;

    BYTE idx = 0;

    if (!strcmp(arg1, "st"))
        idx = POINT_ST;
    else if (!strcmp(arg1, "dx"))
        idx = POINT_DX;
    else if (!strcmp(arg1, "ht"))
        idx = POINT_HT;
    else if (!strcmp(arg1, "iq"))
        idx = POINT_IQ;
    else
        return;

    if (ch->GetRealPoint(idx) >= MAX_STAT)
        return;

    if (idx == POINT_IQ)
    {
        ch->PointChange(POINT_MAX_HP, 0);
    }
    else if (idx == POINT_HT)
    {
        ch->PointChange(POINT_MAX_SP, 0);
    }

    BYTE hstatu = MAX_STAT - ch->GetRealPoint(idx);
    if (hstatu <= ch->GetPoint(POINT_STAT))
    {
        ch->SetRealPoint(idx, ch->GetRealPoint(idx) + hstatu);
        ch->SetPoint(idx, ch->GetPoint(idx) + hstatu);
        ch->ComputePoints();
        ch->PointChange(idx, 0);
        ch->PointChange(POINT_STAT, -hstatu);
        ch->ComputePoints();
    }
    else
    {
        ch->SetRealPoint(idx, ch->GetRealPoint(idx) + ch->GetPoint(POINT_STAT));
        ch->SetPoint(idx, ch->GetPoint(idx) + ch->GetPoint(POINT_STAT));
        ch->ComputePoints();
        ch->PointChange(idx, 0);
        ch->PointChange(POINT_STAT, -ch->GetPoint(POINT_STAT));
        ch->ComputePoints();
    }
}

ACMD(do_stat_val)
{
	char	arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	int val = 0;
	str_to_number(val, arg2);
	
	if (!*arg1 || val <= 0)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu iti poti schimba starea atata timp cat esti transformat."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;
	
	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;
	
	if (val > ch->GetPoint(POINT_STAT))
		val = ch->GetPoint(POINT_STAT);
	
	if (ch->GetRealPoint(idx) + val > MAX_STAT)
		val = MAX_STAT - ch->GetRealPoint(idx);

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + val);
	ch->SetPoint(idx, ch->GetPoint(idx) + val);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
		ch->PointChange(POINT_MAX_HP, 0);
	else if (idx == POINT_HT)
		ch->PointChange(POINT_MAX_SP, 0);

	ch->PointChange(POINT_STAT, -val);
	ch->ComputePoints();
}

ACMD(do_test_edit_delete_item)
{
	char arg1[150];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	
	int iPos = 0;
	str_to_number(iPos, arg1);
	
	if (iPos < 0)
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (!ch->IsOwnerEditShopOffline())
		return;
	// PREVENT BUG WHILE EDIT MODE
	
	COfflineShopManager::instance().DelSingleItem(ch, iPos);
}

ACMD(do_test_edit_price_item)
{
	const char *line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));
	
	if (!*arg1 || !*arg2 || !*arg3)
		return;
	
	int iPos, iPrice = 0;

	str_to_number(iPos, arg1);
	str_to_number(iPrice, arg2);
	
	if (iPos < 0 || iPrice <= 0)
		return;
	
	BYTE bCheque = 0;
	str_to_number(bCheque, arg3);
	
	if (bCheque < 0)
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (!ch->IsOwnerEditShopOffline())
		return;
	// PREVENT BUG WHILE EDIT MODE

	COfflineShopManager::instance().EditPriceItem(ch, iPos, iPrice, bCheque);
}

#include <string>
#include <boost/algorithm/string.hpp>

ACMD(do_test_put_item)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	std::vector<std::string> args;
	boost::split(args, arg1, boost::is_any_of("|"));
	
	if (args.size() < 5)
		return;

	// PREVENT BUG WHILE EDIT MODE
	if (!ch->IsOwnerEditShopOffline())
		return;
	// PREVENT BUG WHILE EDIT MODE
	
	int x, iPrice = 0;
	BYTE bWindow, bCell, bTargetPos, bCheque = 0;

	str_to_number(bWindow, args[x++].c_str());
	str_to_number(bCell, args[x++].c_str());
	str_to_number(bTargetPos, args[x++].c_str());
	str_to_number(iPrice, args[x++].c_str());
	str_to_number(bCheque, args[x++].c_str());
		
	TItemPos item_pos;
	item_pos.window_type = bWindow;
	item_pos.cell = bCell;
		
	COfflineShopManager::instance().PutItemPos(ch, item_pos, bTargetPos, iPrice, bCheque);
}

ACMD(do_test_edit_mode)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;
	
	if (!ch->CanHandleItem())
		return;
	
	// PREVENT BUG WHILE EDIT MODE
	// if (!ch->IsOwnerEditShopOffline())
		// return;
	// PREVENT BUG WHILE EDIT MODE
	
	COfflineShopManager::instance().SendItemsEditMode(ch);	
}

ACMD(do_close_edit_mode)
{
	// char arg1[150];
	// one_argument(argument, arg1, sizeof(arg1));

	// if (!*arg1)
		// return;
	
	// bool bStatus = 0;
	// str_to_number(bStatus, arg1);

	ch->SetEditOfflineShopMode(false);
}

ACMD(do_create_offline_shop_mode)
{
	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(5))
	{
		// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attendi un attimo."));
		return;
	}

	char arg1[150];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	
	bool bStatus = 0;
	str_to_number(bStatus, arg1);

	ch->SetOfflineShopModeCreate(bStatus);
	ch->SetMyOfflineShopTime();
}

ACMD(do_test_destroy_offline_shop)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;
	
	if (!ch->CanHandleItem())
		return;
	
	if (ch->IsHack())
		return;
	
	COfflineShopManager::instance().AutoCloseOfflineShop(ch);	
}

ACMD(do_open_refine_ds)
{
	if (ch)
		ch->DragonSoul_RefineWindow_Open(NULL);
}

#ifdef REMOVE_BUFFS_OII
ACMD(do_remove_buff)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	
	if (!*arg1)
		return;
		
	if (!ch)
		return;
		
	int affect = 0;
	str_to_number(affect, arg1);
	CAffect* pAffect = ch->FindAffect(affect);
	
	if(pAffect)
		ch->RemoveAffect(affect);
}
#endif

#ifdef ENABLE_DECORUM
ACMD(do_decorum_stat)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));
	
	if (!*arg1)
		return;
		
	DWORD vid;
	str_to_number(vid, arg1);
		
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);
	if (NULL == pkVictim)
		return;
		
	DECORUM * pkDec = CDecorumManager::instance().GetDecorum(pkVictim->GetPlayerID());
	if (NULL == pkDec || !pkDec->IsDecored())
		return;

	pkDec->SendDecorumData(ch, vid);
}

ACMD(do_decorum_block)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));
	
	if (!*arg1)
		return;
		
	DWORD block;
	str_to_number(block, arg1);
		
	DECORUM * pkDec = CDecorumManager::instance().GetDecorum(ch->GetPlayerID());
	if (NULL == pkDec || !pkDec->IsDecored())
		return;
		
	pkDec->ChangeBlock(1 << block);
	pkDec->SendDecorumBase(ch);
}

ACMD(do_decorum_arena_accept)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));
	
	if (!*arg1)
		return;
		
	DWORD dwArenaID;
	str_to_number(dwArenaID, arg1);
		
	TPacketGGDecorumArenaBroadcast p;
	p.bHeader = HEADER_GG_DECORUM_RANDOM_BROADCAST;
	p.dwArenaID = dwArenaID;
	p.PacketType = DECORUM_RANDOM_ARENA_BROADCAST_CG;
	p.dwArgument = ch->GetPlayerID();
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGDecorumArenaBroadcast));
	
	//CDecoredArenaManager::instance().InsertRequestApplicant(ch->GetPlayerID());
}
#endif

#include <string>
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
ACMD(search_drop)
{
	int iWaitMadafaka = ch->GetQuestFlag("search.sijaja");
	if (iWaitMadafaka)
	{
		if (get_global_time() < iWaitMadafaka + 3) 
		{
			// ch->ChatPacket(CHAT_TYPE_INFO, "Have to wait 3 seconds to search again");
			return;
		}
	}
	char arg1[4096];
	one_argument(argument, arg1, sizeof(arg1));
	
	if (!*arg1)
		return;

	ITEM_MANAGER::instance().FindItemMonster(ch, arg1);
	ch->SetQuestFlag("search.sijaja", get_global_time());
}

#ifdef GUILD_WAR_COUNTER
ACMD(do_guild_war_static)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	if (vecArgs[1] == "load")
	{
		if (ch->GetProtectTime("update_static_load") == 0)
		{
			if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
			{
				CWarMap* pMap = CWarMapManager::instance().Find(ch->GetMapIndex());
				if (pMap)
				{
					std::vector<war_static_ptr> list;
					pMap->UpdateStatic(ch, GUILD_STATIC_LOAD, list);
					ch->SetProtectTime("update_static_load", 1);
				}
			}
		}
	}
	else if (vecArgs[1] == "spy")
	{
		if (!ch->GetGuild())
			return;

		if (!(ch->GetGuild() && ch->GetGuild()->GetMasterPID() == ch->GetPlayerID()))
			return;
		if (vecArgs.size() < 4) { return; }

		DWORD spy_pid;
		str_to_number(spy_pid, vecArgs[3].c_str());
		ch->GetGuild()->RequestRemoveMember(spy_pid);

		LPCHARACTER spy = CHARACTER_MANAGER::Instance().FindByPID(spy_pid);
		if (spy)
		{
			if (CWarMapManager::instance().IsWarMap(spy->GetMapIndex()))
				spy->ExitToSavedLocation();
		}

		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
		{
			CWarMap* pMap = CWarMapManager::instance().Find(ch->GetMapIndex());
			if (pMap)
				pMap->UpdateSpy(spy_pid);
		}
	}
	else if (vecArgs[1] == "info")
	{
		/*
		if (ch->GetProtectTime("guild.statisticsInfo") == 1)
		{
			CGuildManager::Instance().SendWarStatisticsPacket(ch, GUILD_STATIC_EVENT);
			return;
		}
		ch->SetProtectTime("guildStatisticsInfo", 1);
		*/
		CGuildManager::Instance().SendWarStatisticsPacket(ch, GUILD_STATIC_INFO);
	}
	else if (vecArgs[1] == "data")
	{
		if (vecArgs.size() < 3) { return; }
		DWORD statisticsWarID;
		str_to_number(statisticsWarID, vecArgs[2].c_str());

		char buf[50];
		snprintf(buf, sizeof(buf), "guildStatistics%dData", statisticsWarID);
		if (ch->GetProtectTime(buf) == 1)
		{
			CGuildManager::Instance().SendWarStatisticsPacket(ch, GUILD_STATIC_EVENT);
			return;
		}
		ch->SetProtectTime(buf, 1);
		CGuildManager::Instance().SendWarStatisticsData(ch, statisticsWarID);
	}
}
#endif

#ifdef ENABLE_GUILD_ONLINE_LIST
ACMD(do_guildlist)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "load")
	{
		CGuildManager::Instance().SendOnlineGuildData(ch);
	}
}
#endif


ACMD(do_mount_target)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "0")
	{
		extern void do_ride(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
		do_ride(ch, NULL, 0, 0);
	}
	else if (vecArgs[1] == "1")
	{
		extern void do_user_horse_back(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
		do_user_horse_back(ch, NULL, 0, 0);
	}
}

#ifdef ENABLE_ANTI_EXP
ACMD(do_anti_exp)
{
	time_t real_time = time(0);
	if (ch->GetProtectTime("anti.exp") > real_time)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Devi aspettare %d s.", ch->GetProtectTime("anti.exp") - real_time);
		return;
	}
	ch->SetProtectTime("anti.exp", real_time + 1);
	ch->SetAntiExp(!ch->GetAntiExp());
}
#endif

ACMD(do_daily_reward_reload){
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem DeleteRewards|");
	char* time;
	char* rewards;
	char* items;
	char* counts;
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time),reward from player.daily_reward_status where pid = %u",ch->GetPlayerID()));
	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0){
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL){
			time = row[0];
			rewards = row[1];
		}
	}
	else{
		DBManager::Instance().DirectQuery("INSERT INTO player.daily_reward_status (pid,time,reward,total_rewards) VALUES(%u,NOW(),0,0)", ch->GetPlayerID());
		SQLMsg * pkMsg2(DBManager::instance().DirectQuery("SELECT UNIX_TIMESTAMP(time), reward from player.daily_reward_status where pid = %u",ch->GetPlayerID()));
		SQLResult * pRes2 = pkMsg2->Get();
		if (pRes2->uiNumRows > 0){
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes2->pSQLResult)) != NULL){
				time = row[0];
				rewards = row[1];
			}
		}
	}

	SQLMsg * pkMsg3(DBManager::instance().DirectQuery("SELECT items, count from player.daily_reward_items where reward = '%s'",rewards));
	SQLResult * pRes3 = pkMsg3->Get();
	if (pRes3->uiNumRows > 0){
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes3->pSQLResult)) != NULL){
			items = row[0];
			counts = row[1];
			ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetReward|%s|%s",items,counts);
		}
	}
	
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetTime|%s",time);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetDailyReward|%s", rewards);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ManagerGiftSystem SetRewardDone|");
}

ACMD(do_daily_reward_get_reward){
	if (!ch)
		return;

	char* items;
	char* counts;
	DWORD item;
	DWORD count;
	bool reward = false;
	char* rewards;
	// and (NOW() - interval 30 minute > time) 
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT reward from player.daily_reward_status where (NOW() > time) and pid = %u", ch->GetPlayerID()));
	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0){
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL){
			rewards = row[0];
		}
		reward = true;
	}
	
	if (reward){
		SQLMsg * pkMsg2(DBManager::instance().DirectQuery("SELECT items, count from player.daily_reward_items where reward = '%s' ORDER BY RAND() limit 1",rewards));
		SQLResult * pRes2 = pkMsg2->Get();
		if (pRes2->uiNumRows > 0){
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes2->pSQLResult)) != NULL){
				items = row[0];
				counts = row[1];
			}
		}
		str_to_number(item, items);
		str_to_number(count, counts);
		ch->AutoGiveItem(item, count);
		// ch->ChatPacket(CHAT_TYPE_INFO, "recompensa: %s",items);
		DBManager::Instance().DirectQuery("UPDATE daily_reward_status SET reward = CASE WHEN reward = 0 THEN '1' WHEN reward = 1 THEN '2' WHEN reward = 2 THEN '3' WHEN reward = 3 THEN '4' WHEN reward = 4 THEN '5' WHEN reward = 5 THEN '6' WHEN reward = 6 THEN '0' END, total_rewards = total_rewards +1, time = (NOW() + interval 1 day) WHERE pid = %u", ch->GetPlayerID());
	}
	else{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Abia dupa expirarea celor 24 de ore, puteti colecta un alt premiu."));
	}
}	

#ifdef FAST_EQUIP_WORLDARD

ACMD(do_open_change_equip)
{

	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));
	
	if (0 == arg1[0])
		return;

	int page_index = atoi(arg1);

	if(page_index <= 0 || page_index > CHANGE_EQUIP_PAGE_EXTRA){
		return;
	}

	if(!ch->CanHandleItem()){
		return;
	}

	if (ch->IsDead()){
		return;
	}

	if (ch->IsStun()){
		return;
	}

	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen())
	{
		return;
	}

    int get_time_change_equip = ch->GetTimeChangeEquip();
    int currentPulse = thecore_pulse();
   
    if (get_time_change_equip > currentPulse) {
        int deltaInSeconds = ((get_time_change_equip / PASSES_PER_SEC(1)) - (currentPulse / PASSES_PER_SEC(1)));
        int minutes = deltaInSeconds / 60;
        int seconds = (deltaInSeconds - (minutes * 60));
		
		ch->ChatPacket(CHAT_TYPE_INFO, "you have to wait %02d seconds to change equip .", seconds);
        return;
    }

	DWORD dwCurTime = get_dword_time();

	if (dwCurTime - ch->GetLastAttackTime() <= 1500 || dwCurTime - ch->GetLastSkillTime() <= 1500)
	{
		return;
	}

	DWORD index_old = CHANGE_EQUIP_SLOT_COUNT-(CHANGE_EQUIP_SLOT_COUNT/page_index);

	if(page_index > 1){
		index_old = CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*(page_index-1);
	}

	// Fix Weapon Costume

	LPITEM item_change_equip;
	LPITEM item_inv;
	LPITEM item_extra;

	item_inv = ch->GetWear(WEAR_COSTUME_WEAPON);
	if(item_inv){
		item_change_equip = ch->GetChangeEquipItem(index_old+WEAR_WEAPON);
		if(item_change_equip)
		{
			if (item_change_equip->GetType() != ITEM_WEAPON || item_inv->GetValue(3) != item_change_equip->GetSubType())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Este cambio de equipo no se puede realizar, tu skin de arma actual es incompatible.");
				return;
			}
		}
	}


	item_inv = ch->GetWear(WEAR_WEAPON);
	if(item_inv){
		item_change_equip = ch->GetChangeEquipItem(index_old+WEAR_COSTUME_WEAPON);
		item_extra = ch->GetChangeEquipItem(index_old+WEAR_WEAPON);

		if(item_change_equip && !item_extra)
		{
			if (item_change_equip->GetSubType() != COSTUME_WEAPON || item_change_equip->GetValue(3) != item_inv->GetSubType())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Este cambio de equipo no se puede realizar, tu skin de arma actual es incompatible.");
				return;
			}
		}
	}else{
		item_change_equip = ch->GetChangeEquipItem(index_old+WEAR_COSTUME_WEAPON);
		if(item_change_equip){
			ch->ChatPacket(CHAT_TYPE_INFO, "Este cambio de equipo no se puede realizar, tu skin de arma actual es incompatible.");
			return;	
		}
	}

	// Fix Weapon Costume

	for (int i = index_old; i < CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*page_index; ++i)
	{
		item_change_equip = ch->GetChangeEquipItem(i);

		int cell = i;

		if(page_index > 1){
			if(cell >= CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*(page_index-1) && cell < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(page_index))
			{
				cell = cell - ((CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(page_index-1));
			}
		}

		item_inv = ch->GetWear(cell);

		if(item_change_equip && item_inv == NULL)
		{
			item_change_equip->EquipTo(ch, item_change_equip->FindEquipCell(ch));
		}

		if(item_change_equip && item_inv)
		{
			item_inv->RemoveFromCharacter();
			if(item_change_equip->EquipTo(ch, item_change_equip->FindEquipCell(ch)))
				item_inv->AddToCharacter(ch, TItemPos(CHANGE_EQUIP, i));
			
		}
	}

	ch->SetTimeChangeEquip(thecore_pulse() + PASSES_PER_SEC(7));

}

#endif
