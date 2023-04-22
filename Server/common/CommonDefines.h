#ifndef __INC_METIN2_COMMON_DEFINES_H__
#define __INC_METIN2_COMMON_DEFINES_H__
#pragma once
//////////////////////////////////////////////////////////////////////////
// ### Standard Features ###
#define _IMPROVED_PACKET_ENCRYPTION_
#define __PET_SYSTEM__
#define __UDP_BLOCK__
//#define ENABLE_QUEST_CATEGORY

// ### END Standard Features ###
//////////////////////////////////////////////////////////////////////////

#define M2S_BIO_SYSTEM //Biolog Rework

//////////////////////////////////////////////////////////////////////////
// ### New Features ###
#define ENABLE_NEWGUILDMAKE
#define ENABLE_D_NJGUILD
#define ENABLE_FULL_NOTICE
#define ENABLE_NEWSTUFF
#define ENABLE_PORT_SECURITY
#define ENABLE_BELT_INVENTORY_EX
#define ENABLE_CMD_WARP_IN_DUNGEON
// #define ENABLE_ITEM_ATTR_COSTUME
// #define ENABLE_SEQUENCE_SYSTEM
#define ENABLE_PLAYER_PER_ACCOUNT5
#define ENABLE_DICE_SYSTEM
#define ENABLE_EXTEND_INVEN_SYSTEM
#define ENABLE_MOUNT_COSTUME_SYSTEM
#define ENABLE_WEAPON_COSTUME_SYSTEM
#define ENABLE_QUEST_DIE_EVENT
#define ENABLE_QUEST_BOOT_EVENT
#define ENABLE_QUEST_DND_EVENT
#define ENABLE_EXPANDED_ITEM_NAME

enum eCommonDefines {
	MAP_ALLOW_LIMIT = 32, // 32 default
};

//#define ENABLE_WOLFMAN_CHARACTER
#ifdef ENABLE_WOLFMAN_CHARACTER
#define USE_MOB_BLEEDING_AS_POISON
#define USE_MOB_CLAW_AS_DAGGER
// #define USE_ITEM_BLEEDING_AS_POISON
// #define USE_ITEM_CLAW_AS_DAGGER
#define USE_WOLFMAN_STONES
#define USE_WOLFMAN_BOOKS
#endif

// #define ENABLE_MAGIC_REDUCTION_SYSTEM
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
// #define USE_MAGIC_REDUCTION_STONES
#endif

// ### END New Features ###
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// ### Ex Features ###
#define DISABLE_STOP_RIDING_WHEN_DIE //	if DISABLE_TOP_RIDING_WHEN_DIE is defined, the player doesn't lose the horse after dying
#define ENABLE_ACCE_COSTUME_SYSTEM //fixed version
// #define USE_ACCE_ABSORB_WITH_NO_NEGATIVE_BONUS //enable only positive bonus in acce absorb
#define ENABLE_HIGHLIGHT_NEW_ITEM //if you want to see highlighted a new item when dropped or when exchanged
#define __ENABLE_KILL_EVENT_FIX__ //if you want to fix the 0 exp problem about the when kill lua event (recommended)
// #define ENABLE_SYSLOG_PACKET_SENT // debug purposes

// ### END Ex Features ###
//////////////////////////////////////////////////////////////////////////

#endif
//martysama0134's 2022


//Robert
#define __ITEM_SOCKET5__
#define __EXTENDED_BLEND__
#define __NEW_BLEND_AFFECT__ // New blend affect
#ifdef __NEW_BLEND_AFFECT__
#define __EXTENDED_BLEND_AFFECT__ // Extended blend affect
#endif
//Robert

