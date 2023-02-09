#ifndef __INC_METIN2_COMMON_DEFINES_H__
#define __INC_METIN2_COMMON_DEFINES_H__
#define ENABLE_FIRST_QUEST_THEN_SHOP
#define __FROZENBONUS_SYSTEM__
#define ENABLE_GENERAL_IN_GUILD

//#define __ENABLE_ITEM_GARBAGE__

//////////////////////////////////////////////////////////////////////////
// ### General Features ###
//#define ENABLE_QUEST_CATEGORY
#define ENABLE_D_NJGUILD
#define ENABLE_FULL_NOTICE
#define ENABLE_ANTI_EXP
#define ENABLE_NEWSTUFF
#define ENABLE_NEWGUILDMAKE
#define ENABLE_MULTI_LANGUAGE_SYSTEM
#define ENABLE_PORT_SECURITY
#define ENABLE_BELT_INVENTORY_EX
#define ENABLE_DS_ACTIVE_LOOP
#define NEW_DSS
enum eCommonDefines {
	MAP_ALLOW_LIMIT = 64, // 32 default
};
// ### General Features ###
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// ### CommonDefines Systems ###
// #define ENABLE_WOLFMAN_CHARACTER
#ifdef ENABLE_WOLFMAN_CHARACTER
#define USE_MOB_BLEEDING_AS_POISON
#define USE_MOB_CLAW_AS_DAGGER
// #define USE_ITEM_BLEEDING_AS_POISON
// #define USE_ITEM_CLAW_AS_DAGGER
#define USE_WOLFMAN_STONES
#define USE_WOLFMAN_BOOKS
#endif
#define ENABLE_MOUNT_COSTUME_SYSTEM
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	#define ENABLE_AUTO_RIDE
#endif
#define ENABLE_PLAYER_PER_ACCOUNT5
#define ENABLE_DICE_SYSTEM
#define ENABLE_EXTEND_INVEN_SYSTEM
#define ENABLE_WEAPON_COSTUME_SYSTEM
#define ENABLE_EXPANDED_ITEM_NAME
#define ENABLE_MAGIC_REDUCTION_SYSTEM
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
// #define USE_MAGIC_REDUCTION_STONES //enable for stone bonus
#endif
#define ENABLE_ACCE_COSTUME_SYSTEM
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	#define UNFROZEN_SASHES
#endif
#define TARGET_INFORMATION_SYSTEM
#define __SPECIAL_STORAGE_SYSTEM__
// ### CommonDefines Systems ###
//////////////////////////////////////////////////////////////////////////
#define ENABLE_SWITCHBOT

#define M2S_BIO_SYSTEM

//#define ENABLE_DECORUM
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
	#define MAP_INDEX_ARENA_NO_5VS5 50
	#define MAP_INDEX_ARENA_5VS5 50
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

#endif


