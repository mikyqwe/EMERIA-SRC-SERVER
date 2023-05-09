#ifndef __INC_SERVICE_H__
#define __INC_SERVICE_H__
#pragma once
#include "CommonDefines.h"
#endif

//Rework --- START

//OFFLINESHOP
#define __ENABLE_NEW_OFFLINESHOP__
#define ENABLE_NEW_OFFLINESHOP_LOGS
#ifdef __ENABLE_NEW_OFFLINESHOP__
#define ENABLE_NEW_SHOP_IN_CITIES
#endif

#define ENABLE_LONG_LONG    //yang over 2kkk

#define ENABLE_NEW_OFFLINESHOP_RENEWAL
#define KASMIR_PAKET_SYSTEM

//#define ENABLE_OFFLINESHOP_DEBUG
#ifdef ENABLE_OFFLINESHOP_DEBUG
#	ifdef __WIN32__
#		define OFFSHOP_DEBUG(fmt , ...) sys_log(0,"%s:%d >> " fmt , __FUNCTION__ , __LINE__, __VA_ARGS__)
#	else
#		define OFFSHOP_DEBUG(fmt , args...) sys_log(0,"%s:%d >> " fmt , __FUNCTION__ , __LINE__, ##args)
#	endif
#else
#	define OFFSHOP_DEBUG(...)   
#endif

//*________________________________________________________________________________________________/**

//* Bug Fixes
//1. class CFuncViewInsert: Rework sectree memory
//2. CancelServerTimers
//3. WARNING!!! (only if you care about multilanguage, otherwise ignore): const char * GetName(); edit this func in item.h for make it work with multilangue for item on the ground.
//Also for switchbot item: void CSwitchbot::SwitchItems() the GetName Function.

//* Pickup Renewal
#define __BL_OFFICIAL_LOOT_FILTER__
#if defined(__BL_OFFICIAL_LOOT_FILTER__)
#	define __PREMIUM_LOOT_FILTER__ // Enable Premium Usage of the Loot Filter System
#endif


//* General Features 
#define ENABLE_MULTI_LANGUAGE_SYSTEM
#define __QUEST_RENEWAL__
#define ENABLE_NEW_DETAILS_GUI
#define _QR_MS_
#define __NEW_EXCHANGE_WINDOW__
#define UNFROZEN_SASHES
#define __DUNGEON_INFO_SYSTEM__
#define ENABLE_UNLIMITED_ARGUMENT  // for cmd argument unlimited
#define ENABLE_EXTENDED_ITEMNAME_ON_GROUND
#define ENABLE_ANTI_EXP
#define ENABLE_ANTI_MULTIPLE_FARM
//#define ENABLE_DS_ACTIVE_LOOP
#define ELEMENT_TARGET

//* QoL Rework
#define ENABLE_AGGREGATE_MONSTER_EFFECT
#define WJ_ENABLE_TRADABLE_ICON
#define ENABLE_GIVE_BASIC_WEAPON
#define TARGET_INFORMATION_SYSTEM
#define __SPECIAL_STORAGE_SYSTEM__
#define ENABLE_SELL_ITEM
#define FAST_EQUIP_WORLDARD
#define ENABLE_SWITCHBOT
#define ENABLE_CUBE_RENEWAL_WORLDARD
#define ENABLE_ATLAS_BOSS
#define ITEM_BUFF_SYSTEM

//* Hunting Renewal
#define ENABLE_HUNTING_SYSTEM
#ifdef ENABLE_HUNTING_SYSTEM
	#define HUNTING_MISSION_COUNT 90
	#define HUNTING_MONEY_TABLE_SIZE 9
	#define HUNTING_EXP_TABLE_SIZE 9
#endif

//* Pet System New
#define NEW_PET_SYSTEM