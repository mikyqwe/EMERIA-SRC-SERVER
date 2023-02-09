#ifndef __INC_SERVICE_H__
#define __INC_SERVICE_H__
#define ENABLE_TARGET_HALF_BUFF
#define ENABLE_GLOBAL_CHAT
#define __ULTIMATE_TOOLTIP__
#define __QUEST_RENEWAL__
#define _QR_MS_
#define ENABLE_INSTANT_BONUS
//#define _IMPROVED_PACKET_ENCRYPTION_
#define ENABLE_AFK_MODE_SYSTEM
#define ENABLE_UNLIMITED_ARGUMENT  // for cmd argument unlimited
//#define __AUCTION__
#define __SPECIALSTAT_SYSTEM__
//#define __REMOVE_PARTY_IN_SPECIFIC_MAP
#define __PET_SYSTEM__
#define __UDP_BLOCK__
#define GUILD_WAR_COUNTER
#define ENABLE_AGGREGATE_MONSTER_EFFECT
//#define ENABLE_BIOLOG_SYSTEM
#define ENABLE_AURA_SYSTEM
#ifdef ENABLE_AURA_SYSTEM
	#define CLEAR_BONUS_AURA
#endif
#define __NEW_EVENT_HANDLER__
#define ENABLE_CUBE_RENEWAL_WORLDARD
/* OFFLINE SHOPS & TRADE REWORK MENTAL */
#define OFFLINE_SHOP // Offline shops system
#define OFFLINE_SHOP_BUG_FIX
#define ENABLE_CHEQUE_SYSTEM
#define PRIVATESHOP_SEARCH_SYSTEM
#define _OPEN_SEARCH_WINDOW_ 
#define CHANGELOOK_SYSTEM	
/* END MENTAL REWORK */
//#define HANDSHAKE_FIX
#define FIXED_GUILD_MAX_MEMBERS
#define ENABLE_SWITCHBOT
//#define ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
#define ENABLE_USE_DIFFERENT_TABLE_FOR_COSTUME_ATTRIBUTE
//#define DISABLE_HORSE_STOP_RIDING_IF_DEAD
#define ENABLE_OVER_KILL
//#define NEW_ADD_INVENTORY
#define ENABLE_NEW_TALISMAN_GF
#ifdef ENABLE_NEW_TALISMAN_GF
	#define ELEMENT_TARGET
	//#define ENABLE_NEW_TALISMAN_SLOTS
	#define ENABLE_NEW_TALISMAN_SLOTS_NEW
#endif
#define __ENABLE_TRASH_BIN__
//#define ENABLE_IGNORE_LOWER_BUFFS
#define ENABLE_MESSENGER_TEAM
#define ENABLE_HIDE_COSTUME_SYSTEM
#define ENABLE_HIDE_COSTUME_SYSTEM_ACCE
#define ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
#define __BATTLE_PASS__
#define SHOP_TIME_REFRESH 1*60 
#define SHOP_BLOCK_GAME99
//#define SHOP_DISTANCE
#define SHOP_AUTO_CLOSE
//#define SHOP_ONLY_ALLOWED_INDEX
#define SHOP_HIDE_NAME
#define SHOP_GM_PRIVILEGES GM_IMPLEMENTOR
#define ANTY_WAIT_HACK
#define __NEW_EXCHANGE_WINDOW__
#define __SWAPITEM_SYSTEM__
#define NEW_PET_SYSTEM
#define ENABLE_EXPRESSING_EMOTION
#define ENABLE_MESSENGER_BLOCK
//#define ENABLE_SPECIAL_AFFECT
#define __HIGHLIGHT_SYSTEM__
#define ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
#define ENABLE_6_7_BONUS_NEW_SYSTEM
#define ENABLE_DS_GRADE_MYTH
//#define ENABLE_SORT_INVEN
#define SKILL_COOLTIME_UPDATE
#define ENABLE_SHOW_CHEST_DROP
//#define __BL_SOUL_ROULETTE__
//#define __SOUL_SYSTEM__ // Soul system
#define RENEWAL_DEAD_PACKET
#define ENABLE_EXTENDED_DS_INVENTORY
#define STRONG_AGAINST_MONSTER_BONUS_IN_SKILL_P
#define GUILD_WAR_FIXIN
#define REMOVE_BUFFS_OII
#define PET_SEAL_EFFECT
#define NEW_RING_SLOT
#define WON_EXCHANGE
#define __BL_DROP_DESTROY_TIME__
#define CHANGE_PETSYSTEM
#define UNLIMITED_GREEN_PURPLE_P_A_C
#define PERMA_ELIXIRS_S_M
#endif
#define __INGAME_WIKI__
#define ENABLE_FIX_PET_TRANSPORT_BOX
#define ENABLE_EFFECT_COSTUME_SYSTEM
#define NO_MONARCH_POWERS_IN_WAR //Enable/Disable monarch powers in a Guild War/Decored Arena
#define __ENABLE_ITEM_GARBAGE__
#define __IMPROVED_GUILD_WAR__
#define ENABLE_GUILD_ONLINE_LIST
#define ENABLE_DUEL_EFFECT
//#define __BL_RANKING__
#define GUILD_RANK_SYSTEM
#define GUILD_RANK_EFFECT
#define ENABLE_EVENT_MANAGER
#define __DUNGEON_INFO_SYSTEM__
#define __CHATTING_WINDOW_RENEWAL__
#define __BL_KILL_BAR__
#define ENABLE_HUNTING_SYSTEM
#define __WORLD_BOSS_YUMA__
#define ENABLE_RENEWAL_PVP
//#define ENABLE_MELEY_LAIR_DUNGEON
//#define ENABLE_DEFENSAWE_SHIP
#ifdef ENABLE_HUNTING_SYSTEM
	#define HUNTING_MISSION_COUNT 90
	#define HUNTING_MONEY_TABLE_SIZE 9
	#define HUNTING_EXP_TABLE_SIZE 9
#endif
//#define __BL_PARTY_POSITION__
#define ENABLE_WHISPER_RENEWAL
#define ENABLE_MULTISHOP
#define __NEW_ARROW_SYSTEM__
#define SYSTEM_PDA
#define ITEM_BUFF_SYSTEM
#define ENABLE_POLY_SHOP
#define ENABLE_AFFECT_POLYMORPH_REMOVE
#define ENABLE_MAINTENANCE_SYSTEM
#define BL_REMOTE_SHOP
#define ENABLE_REAL_TIME_ENCHANT
#define ENABLE_NEW_AFFECT_POTION
//#define u1x
#define ENABLE_ANTI_MULTIPLE_FARM
#define ENABLE_SEND_TARGET_INFO_EXTENDED
#define ENABLE_BONUS_BOSS
#define __ENABLE_KILL_EVENT_FIX__ //if you want to fix the 0 exp problem about the when kill lua event (recommended)
#define ENABLE_KILL_STATISTICS
#define ENABLE_NEW_DETAILS_GUI
#define ENABLE_BONUS_METIN
#define ENABLE_GOTO_LAG_FIX
#define KILL_AURA_FIX //Fix Kill-Aura Long Distance Bot


#define ENABLE_DECORUM
#ifdef ENABLE_DECORUM
	#define MIN_LEVEL_RANGE_LOBBY_APPLICANT 10 //event_flag: arena_level_limitation
	#define MAX_LEVEL_RANGE_LOBBY_APPLICANT 10 //event_flag: arena_level_limitation
	#define ARENA_LEVEL_LIMITATION 120 //event_flag: arena_level_limitation
	#define BONUS_DECORUM_1 6 //MAX HP
	#define BONUS_DECORUM_QTY_1 2000
	#define BONUS_DECORUM_2 121 //ABI
	#define BONUS_DECORUM_QTY_2 5
	#define BONUS_DECORUM_3 122 //MEDI
	#define BONUS_DECORUM_QTY_3 5
	#define MAP_INDEX_ARENA_NO_5VS5 103
	#define MAP_INDEX_ARENA_5VS5 103
	#define DISABLE_AUTOPOT_DECORUM //Enable/Disable autopotions in a decored arena
	#define DISABLE_FISH_DECORUM //Enable/Disable fishes in a decored arena
	#define DISABLE_HORSE_DECORUM //Enable/Disable the horse in a decored arena
	#define DISABLE_EMOTIONS_DECORUM //Enable/Disable emotions in a decored arena
	#define DISABLE_PET_MOUNT_DECORUM //Enable/Disable mounting a pet in a decored arena
	/************************************************************************************************************************************/
	/*	decorum.cpp -> DECORUM::CheckLegueLevel																							*/
	/*	I bonus sono dati secondo fLeagueRate.																							*/
	/*	float fLegueRate = (dwLegue - 4) / ((float)LEGUE_MAX_LEVEL - 4 - 1);															*/
	/*	pkChr->AddAffect(AFFECT_DECORUM, POINT_MAX_HP, 2000 * fLegueRate, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);					*/
	/*	pkChr->AddAffect(AFFECT_DECORUM, POINT_SKILL_DAMAGE_BONUS, 5 * fLegueRate, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);		*/
	/*	pkChr->AddAffect(AFFECT_DECORUM, POINT_NORMAL_HIT_DAMAGE_BONUS, 5 * fLegueRate, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false);	*/
	/************************************************************************************************************************************/
#endif

// HACKSHIELD //**NOT ENABLED**//

//#define ENABLE_CSHIELD
#ifdef ENABLE_CSHIELD
	#define ENABLE_AUTOBAN // enables autoban if player has too many CShield Async reports

	#define ENABLE_CHECK_ATTACKSPEED_HACK //checks if player is attacking too fast
	#define ENABLE_CHECK_MOVESPEED_HACK //checks if player is moving too fast
	#define ENABLE_CHECK_WAIT_HACK // checks if player is using waithack


	// enable this checks if you have time to test it
/*
	#define ENABLE_CHECK_PICKUP_HACK 100 //checks if player using pickup hack //important: check CPythonPlayer::SendClickItemPacket for pickup time. if you dont have that just disable this define
	#define ENABLE_CHECK_GHOSTMODE //checks if player is using ghostmode
	#define ENABLE_CHECK_WALLHACK //checks player position for wallhack
*/
#endif
