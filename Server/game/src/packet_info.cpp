#include "stdafx.h"
#include "../../common/stl.h"
#include "constants.h"
#include "packet_info.h"
#ifdef __INGAME_WIKI__
	#include "../../common/in_game_wiki.h"
#endif
#include "HackShield_Impl.h"
#include "XTrapManager.h"
#include "../../common/CommonDefines.h"
#if defined(__DUNGEON_INFO_SYSTEM__)
#	include "dungeon_info.h"
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"
#endif
CPacketInfo::CPacketInfo()
	: m_pCurrentPacket(NULL), m_dwStartTime(0)
{
}

CPacketInfo::~CPacketInfo()
{
	itertype(m_pPacketMap) it = m_pPacketMap.begin();
	for ( ; it != m_pPacketMap.end(); ++it) {
		M2_DELETE(it->second);
	}
}

void CPacketInfo::Set(int header, int iSize, const char * c_pszName)
{
	if (m_pPacketMap.find(header) != m_pPacketMap.end())
		return;

	TPacketElement * element = M2_NEW TPacketElement;

	element->iSize = iSize;
	element->stName.assign(c_pszName);
	element->iCalled = 0;
	element->dwLoad = 0;
/*
	element->bSequencePacket = bSeq;

	if (element->bSequencePacket)
		element->iSize += sizeof(BYTE);
*/
	m_pPacketMap.insert(std::map<int, TPacketElement *>::value_type(header, element));
}

bool CPacketInfo::Get(int header, int * size, const char ** c_ppszName)
{
	std::map<int, TPacketElement *>::iterator it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return false;

	*size = it->second->iSize;
	*c_ppszName = it->second->stName.c_str();

	m_pCurrentPacket = it->second;
	return true;
}

/* bool CPacketInfo::IsSequence(int header)
{
	TPacketElement * pkElement = GetElement(header);
	return pkElement ? pkElement->bSequencePacket : false;
}

void CPacketInfo::SetSequence(int header, bool bSeq)
{
	TPacketElement * pkElem = GetElement(header);

	if (pkElem)
	{
		if (bSeq)
		{
			if (!pkElem->bSequencePacket)
				pkElem->iSize++;
		}
		else
		{
			if (pkElem->bSequencePacket)
				pkElem->iSize--;
		}

		pkElem->bSequencePacket = bSeq;
	}
} */

TPacketElement * CPacketInfo::GetElement(int header)
{
	std::map<int, TPacketElement *>::iterator it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return NULL;

	return it->second;
}

void CPacketInfo::Start()
{
	assert(m_pCurrentPacket != NULL);
	m_dwStartTime = get_dword_time();
}

void CPacketInfo::End()
{
	++m_pCurrentPacket->iCalled;
	m_pCurrentPacket->dwLoad += get_dword_time() - m_dwStartTime;
}

void CPacketInfo::Log(const char * c_pszFileName)
{
	FILE * fp;

	fp = fopen(c_pszFileName, "w");

	if (!fp)
		return;

	std::map<int, TPacketElement *>::iterator it = m_pPacketMap.begin();

	fprintf(fp, "Name             Called     Load       Ratio\n");

	while (it != m_pPacketMap.end())
	{
		TPacketElement * p = it->second;
		++it;

		fprintf(fp, "%-16s %-10d %-10u %.2f\n",
				p->stName.c_str(),
				p->iCalled,
				p->dwLoad,
				p->iCalled != 0 ? (float) p->dwLoad / p->iCalled : 0.0f);
	}

	fclose(fp);
}
///---------------------------------------------------------

CPacketInfoCG::CPacketInfoCG()
{
#ifdef ENABLE_ANTI_MULTIPLE_FARM
	Set(HEADER_CG_ANTI_FARM, sizeof(TSendAntiFarmInfo), "RecvAntiFarmUpdateStatus");
#endif
	Set(HEADER_CG_TEXT, sizeof(TPacketCGText), "Text");
	Set(HEADER_CG_HANDSHAKE, sizeof(TPacketCGHandshake), "Handshake");
	Set(HEADER_CG_TIME_SYNC, sizeof(TPacketCGHandshake), "TimeSync");
	Set(HEADER_CG_MARK_LOGIN, sizeof(TPacketCGMarkLogin), "MarkLogin");
	Set(HEADER_CG_MARK_IDXLIST, sizeof(TPacketCGMarkIDXList), "MarkIdxList");
	Set(HEADER_CG_MARK_CRCLIST, sizeof(TPacketCGMarkCRCList), "MarkCrcList");
	Set(HEADER_CG_MARK_UPLOAD, sizeof(TPacketCGMarkUpload), "MarkUpload");
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	Set(HEADER_CG_KEY_AGREEMENT, sizeof(TPacketKeyAgreement), "KeyAgreement");
#endif

	Set(HEADER_CG_GUILD_SYMBOL_UPLOAD, sizeof(TPacketCGGuildSymbolUpload), "SymbolUpload");
	Set(HEADER_CG_SYMBOL_CRC, sizeof(TPacketCGSymbolCRC), "SymbolCRC");
	Set(HEADER_CG_LOGIN, sizeof(TPacketCGLogin), "Login");
	Set(HEADER_CG_LOGIN2, sizeof(TPacketCGLogin2), "Login2");
	Set(HEADER_CG_LOGIN3, sizeof(TPacketCGLogin3), "Login3");
	Set(HEADER_CG_LOGIN5_OPENID, sizeof(TPacketCGLogin5), "Login5");	//OpenID
	Set(HEADER_CG_ATTACK, sizeof(TPacketCGAttack), "Attack");
	Set(HEADER_CG_CHAT, sizeof(TPacketCGChat), "Chat");
	Set(HEADER_CG_WHISPER, sizeof(TPacketCGWhisper), "Whisper");
	#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	Set(HEADER_CG_REQUEST_FLAG, sizeof(TPacketCGRequestFlag), "TPacketCGRequestFlag");
#endif
	Set(HEADER_CG_CHARACTER_SELECT, sizeof(TPacketCGPlayerSelect), "Select");
	Set(HEADER_CG_CHARACTER_CREATE, sizeof(TPacketCGPlayerCreate), "Create");
	Set(HEADER_CG_CHARACTER_DELETE, sizeof(TPacketCGPlayerDelete), "Delete");
	Set(HEADER_CG_ENTERGAME, sizeof(TPacketCGEnterGame), "EnterGame");

	Set(HEADER_CG_ITEM_USE, sizeof(TPacketCGItemUse), "ItemUse");
	Set(HEADER_CG_ITEM_DROP, sizeof(TPacketCGItemDrop), "ItemDrop");
	Set(HEADER_CG_ITEM_DROP2, sizeof(TPacketCGItemDrop2), "ItemDrop2");
	Set(HEADER_CG_ITEM_MOVE, sizeof(TPacketCGItemMove), "ItemMove");
#ifdef ENABLE_SORT_INVEN
	Set(SORT_INVEN, sizeof(TPacketCGSortInven), "InventorySort");
#endif
#ifdef WON_EXCHANGE
	Set(HEADER_CG_WON_EXCHANGE, sizeof(TPacketCGWonExchange), "WonExchange");
#endif
#ifdef NEW_ADD_INVENTORY
	Set(ENVANTER_BLACK, sizeof(TPacketCGEnvanter), "Envanter");
#endif
	Set(HEADER_CG_ITEM_PICKUP, sizeof(TPacketCGItemPickup), "ItemPickup");

	Set(HEADER_CG_QUICKSLOT_ADD, sizeof(TPacketCGQuickslotAdd), "QuickslotAdd");
#if defined(__BL_SOUL_ROULETTE__)
	Set(HEADER_CG_SOUL_ROULETTE, sizeof(TPacketCGSoulRoulette), "SoulRoulette");
#endif	
	Set(HEADER_CG_QUICKSLOT_DEL, sizeof(TPacketCGQuickslotDel), "QuickslotDel");
	Set(HEADER_CG_QUICKSLOT_SWAP, sizeof(TPacketCGQuickslotSwap), "QuickslotSwap");

	Set(HEADER_CG_SHOP, sizeof(TPacketCGShop), "Shop");

	Set(HEADER_CG_ON_CLICK, sizeof(TPacketCGOnClick), "OnClick");
	Set(HEADER_CG_EXCHANGE, sizeof(TPacketCGExchange), "Exchange");
	Set(HEADER_CG_CHARACTER_POSITION, sizeof(TPacketCGPosition), "Position");
	Set(HEADER_CG_SCRIPT_ANSWER, sizeof(TPacketCGScriptAnswer), "ScriptAnswer");
	Set(HEADER_CG_SCRIPT_BUTTON, sizeof(TPacketCGScriptButton), "ScriptButton");
	Set(HEADER_CG_QUEST_INPUT_STRING, sizeof(TPacketCGQuestInputString), "QuestInputString");
	Set(HEADER_CG_QUEST_CONFIRM, sizeof(TPacketCGQuestConfirm), "QuestConfirm");

	Set(HEADER_CG_MOVE, sizeof(TPacketCGMove), "Move");
	Set(HEADER_CG_SYNC_POSITION, sizeof(TPacketCGSyncPosition), "SyncPosition");

	Set(HEADER_CG_FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "FlyTarget");
	Set(HEADER_CG_ADD_FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "AddFlyTarget");
	Set(HEADER_CG_SHOOT, sizeof(TPacketCGShoot), "Shoot");

	Set(HEADER_CG_USE_SKILL, sizeof(TPacketCGUseSkill), "UseSkill");

	Set(HEADER_CG_ITEM_USE_TO_ITEM, sizeof(TPacketCGItemUseToItem), "UseItemToItem");
	Set(HEADER_CG_TARGET, sizeof(TPacketCGTarget), "Target");
	Set(HEADER_CG_WARP, sizeof(TPacketCGWarp), "Warp");
	Set(HEADER_CG_MESSENGER, sizeof(TPacketCGMessenger), "Messenger");
#ifdef ANTY_WAIT_HACK
	Set(HEADER_CG_ANTY_WAIT_HACK, sizeof(TPacketCGAntyWH), "TPacketCGAntyWH");
#endif

	Set(HEADER_CG_PARTY_REMOVE, sizeof(TPacketCGPartyRemove), "PartyRemove");
	Set(HEADER_CG_PARTY_INVITE, sizeof(TPacketCGPartyInvite), "PartyInvite");
	Set(HEADER_CG_PARTY_INVITE_ANSWER, sizeof(TPacketCGPartyInviteAnswer), "PartyInviteAnswer");
	Set(HEADER_CG_PARTY_SET_STATE, sizeof(TPacketCGPartySetState), "PartySetState");
	Set(HEADER_CG_PARTY_USE_SKILL, sizeof(TPacketCGPartyUseSkill), "PartyUseSkill");
	Set(HEADER_CG_PARTY_PARAMETER, sizeof(TPacketCGPartyParameter), "PartyParam");

	Set(HEADER_CG_EMPIRE, sizeof(TPacketCGEmpire), "Empire");
	Set(HEADER_CG_SAFEBOX_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "SafeboxCheckout");
	Set(HEADER_CG_SAFEBOX_CHECKIN, sizeof(TPacketCGSafeboxCheckin), "SafeboxCheckin");

	Set(HEADER_CG_SAFEBOX_ITEM_MOVE, sizeof(TPacketCGItemMove), "SafeboxItemMove");
#if defined(__BL_MAILBOX__)
	Set(HEADER_CG_MAILBOX_WRITE, sizeof(TPacketCGMailboxWrite), "MailboxWrite");
	Set(HEADER_CG_MAILBOX_WRITE_CONFIRM, sizeof(TPacketCGMailboxWriteConfirm), "MailboxConfirm");
	Set(HEADER_CG_MAILBOX_PROCESS, sizeof(TPacketMailboxProcess), "MailboxProcess");
#endif
#if defined(BL_REMOTE_SHOP)
	Set(HEADER_CG_REMOTE_SHOP, sizeof(TPacketCGRemoteShop), "RemoteShop");
#endif
#ifdef __INGAME_WIKI__
	Set(InGameWiki::HEADER_CG_WIKI, sizeof(InGameWiki::TCGWikiPacket), "RecvWikiPacket");
#endif

	Set(HEADER_CG_GUILD, sizeof(TPacketCGGuild), "Guild");
	Set(HEADER_CG_ANSWER_MAKE_GUILD, sizeof(TPacketCGAnswerMakeGuild), "AnswerMakeGuild");

	Set(HEADER_CG_FISHING, sizeof(TPacketCGFishing), "Fishing");
	Set(HEADER_CG_ITEM_GIVE, sizeof(TPacketCGGiveItem), "ItemGive");
	Set(HEADER_CG_HACK, sizeof(TPacketCGHack), "Hack");
	Set(HEADER_CG_MYSHOP, sizeof(TPacketCGMyShop), "MyShop");
#ifdef CHANGELOOK_SYSTEM
	Set(HEADER_CG_CL, sizeof(TPacketChangeLook), "ChangeLook");
#endif
#ifdef OFFLINE_SHOP
	Set(HEADER_CG_OFFLINE_SHOP, sizeof(TPacketCGShop), "OfflineShop");
	Set(HEADER_CG_MY_OFFLINE_SHOP, sizeof(TPacketCGMyOfflineShop), "MyOfflineShop");
#endif
#ifdef NEW_PET_SYSTEM
	Set(HEADER_CG_PetSetName, sizeof(TPacketCGRequestPetName), "BraveRequestPetName");
#endif
	Set(HEADER_CG_REFINE, sizeof(TPacketCGRefine), "Refine");
	Set(HEADER_CG_CHANGE_NAME, sizeof(TPacketCGChangeName), "ChangeName");
#ifdef ENABLE_CSHIELD
	Set(HEADER_CG_CSHIELD, sizeof(TPacketCGCShield), "CShield");
#endif

	Set(HEADER_CG_CLIENT_VERSION, sizeof(TPacketCGClientVersion), "Version");
	Set(HEADER_CG_CLIENT_VERSION2, sizeof(TPacketCGClientVersion2), "Version");
	Set(HEADER_CG_PONG, sizeof(BYTE), "Pong");
	Set(HEADER_CG_MALL_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "MallCheckout");

	Set(HEADER_CG_SCRIPT_SELECT_ITEM, sizeof(TPacketCGScriptSelectItem), "ScriptSelectItem");
	Set(HEADER_CG_PASSPOD_ANSWER, sizeof(TPacketCGPasspod), "PasspodAnswer");

	Set(HEADER_CG_HS_ACK, sizeof(TPacketGCHSCheck), "HackShieldResponse");
	Set(HEADER_CG_XTRAP_ACK, sizeof(TPacketXTrapCSVerify), "XTrapResponse");
	Set(HEADER_CG_DRAGON_SOUL_REFINE, sizeof(TPacketCGDragonSoulRefine), "DragonSoulRefine");
#ifdef ENABLE_GUILD_REQUEST
	Set(HEADER_CG_GUILD_REQUEST, sizeof(TPacketCGGuildRequest), "GuildRequest");
#endif
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
	Set(HEADER_CG_67_BONUS_NEW, sizeof(TPacketCG67BonusSend), "Bonus67NewSend");
#endif
#ifdef ENABLE_SWITCHBOT
	Set(HEADER_CG_SWITCHBOT, sizeof(TPacketGCSwitchbot), "Switchbot");
#endif
	Set(HEADER_CG_STATE_CHECKER, sizeof(BYTE), "ServerStateCheck");
#ifdef SYSTEM_PDA
	Set(HEADER_CG_SOULSTONE_USE, sizeof(TPacketCGSoulStoneUse), "SoulStoneUse");
#endif
#ifdef __ULTIMATE_TOOLTIP__
	Set(HEADER_CG_CHEST_DROP_INFO, sizeof(TPacketCGChestDropInfo), "ChestDropInfo");
#endif
#ifdef PRIVATESHOP_SEARCH_SYSTEM
	Set(HEADER_CG_PSHOP_SEARCH, sizeof(TPacketCGShopSearchInfo), "ShopSearchInfo");
	Set(HEADER_CG_PSHOP_SEARCH_NAME_ONLY, sizeof(TPacketCGShopSearchInfo), "ShopSearchInfoNameOnly");
	Set(HEADER_CG_PSHOP_SEARCH_BUY, sizeof(TPacketCGShopSearchBuyItem), "ShopSearchBuyItem");
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	Set(HEADER_CG_ACCE, sizeof(TPacketAcce), "Acce");
#endif
#ifdef TARGET_INFORMATION_SYSTEM
	Set(HEADER_CG_TARGET_INFO_LOAD, sizeof(TPacketCGTargetInfoLoad), "TargetInfoLoad");
#endif

#ifdef ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
	Set(HEADER_CG_BUY_BONUS_CHANGER_IN_SWITCH_BOT , sizeof(BYTE) , "BuyChangerBonus");
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	Set(HEADER_CG_CUBE_RENEWAL, sizeof(TPacketCGCubeRenewalSend), "CubeRenewalSend");
#endif
#ifdef ENABLE_HUNTING_SYSTEM
	Set(HEADER_CG_SEND_HUNTING_ACTION, sizeof(TPacketGCHuntingAction), "ReciveHuntingAction");
#endif
#if defined(__DUNGEON_INFO_SYSTEM__)
	Set(DungeonInfo::Packet::HEADER_CG_DUNGEON_INFO, sizeof(DungeonInfo::Packet::CGInfo), "DungeonInfo");
#endif

#ifdef ENABLE_NEW_FISHING_SYSTEM
	Set(HEADER_CG_FISHING_NEW, sizeof(TPacketFishingNew), "PacketFishingNew");
#endif
}

CPacketInfoCG::~CPacketInfoCG()
{
	Log("packet_info.txt");
}

////////////////////////////////////////////////////////////////////////////////
CPacketInfoGG::CPacketInfoGG()
{
#ifdef ENABLE_ANTI_MULTIPLE_FARM
	Set(HEADER_GG_ANTI_FARM, sizeof(CAntiMultipleFarm::TP2PChangeDropStatus), "RecvAntiFarmUpdateStatus");
#endif
	Set(HEADER_GG_SETUP,		sizeof(TPacketGGSetup),		"Setup");
	Set(HEADER_GG_LOGIN,		sizeof(TPacketGGLogin),		"Login");
	Set(HEADER_GG_LOGOUT,		sizeof(TPacketGGLogout),	"Logout");
	Set(HEADER_GG_RELAY,		sizeof(TPacketGGRelay),		"Relay");
	Set(HEADER_GG_NOTICE,		sizeof(TPacketGGNotice),	"Notice");
#ifdef ENABLE_FULL_NOTICE
	Set(HEADER_GG_BIG_NOTICE,	sizeof(TPacketGGNotice),	"BigNotice");
#endif
	Set(HEADER_GG_SHUTDOWN,		sizeof(TPacketGGShutdown),	"Shutdown");
	Set(HEADER_GG_GUILD,		sizeof(TPacketGGGuild),		"Guild");
	Set(HEADER_GG_SHOUT,		sizeof(TPacketGGShout),		"Shout");
	Set(HEADER_GG_DISCONNECT,	    	sizeof(TPacketGGDisconnect),	"Disconnect");
	Set(HEADER_GG_MESSENGER_ADD,	sizeof(TPacketGGMessenger),	"MessengerAdd");
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
	Set(HEADER_GG_MESSENGER_REQUEST_ADD, sizeof(TPacketGGMessengerRequest), "MessengerRequestAdd");
#endif
	Set(HEADER_GG_MESSENGER_REMOVE,	sizeof(TPacketGGMessenger),	"MessengerRemove");
#ifdef ENABLE_MESSENGER_BLOCK
	Set(HEADER_GG_MESSENGER_BLOCK_ADD,	sizeof(TPacketGGMessenger),	"MessengerBlockAdd");
	Set(HEADER_GG_MESSENGER_BLOCK_REMOVE,	sizeof(TPacketGGMessenger),	"MessengerBlockRemove");
#endif
	Set(HEADER_GG_FIND_POSITION,	sizeof(TPacketGGFindPosition),	"FindPosition");
	Set(HEADER_GG_WARP_CHARACTER,	sizeof(TPacketGGWarpCharacter),	"WarpCharacter");
	Set(HEADER_GG_MESSENGER_MOBILE,	sizeof(TPacketGGMessengerMobile), "MessengerMobile");
	Set(HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX, sizeof(TPacketGGGuildWarMapIndex), "GuildWarMapIndex");
	Set(HEADER_GG_TRANSFER,		sizeof(TPacketGGTransfer),	"Transfer");
	Set(HEADER_GG_XMAS_WARP_SANTA,	sizeof(TPacketGGXmasWarpSanta),	"XmasWarpSanta");
	Set(HEADER_GG_XMAS_WARP_SANTA_REPLY, sizeof(TPacketGGXmasWarpSantaReply), "XmasWarpSantaReply");
	Set(HEADER_GG_RELOAD_CRC_LIST,	sizeof(BYTE),			"ReloadCRCList");
	Set(HEADER_GG_CHECK_CLIENT_VERSION, sizeof(BYTE),			"CheckClientVersion");
	Set(HEADER_GG_LOGIN_PING,		sizeof(TPacketGGLoginPing),	"LoginPing");

	// BLOCK_CHAT
	Set(HEADER_GG_BLOCK_CHAT,		sizeof(TPacketGGBlockChat),	"BlockChat");
#ifdef ENABLE_MAINTENANCE_SYSTEM
	Set(HEADER_GG_PLAYER_PACKET, sizeof(TPacketGGPlayerPacket), "PlayerPacket");
#endif
	// END_OF_BLOCK_CHAT
	Set(HEADER_GG_SIEGE,	sizeof(TPacketGGSiege),	"Siege");

	Set(HEADER_GG_MONARCH_NOTICE,		sizeof(TPacketGGMonarchNotice),	"MonarchNotice");
	Set(HEADER_GG_MONARCH_TRANSFER,		sizeof(TPacketMonarchGGTransfer),	"MonarchTransfer");
	Set(HEADER_GG_PCBANG_UPDATE,		sizeof(TPacketPCBangUpdate),		"PCBangUpdate");
	Set(HEADER_GG_CHECK_AWAKENESS,		sizeof(TPacketGGCheckAwakeness),	"CheckAwakeness");
#ifdef __WORLD_BOSS_YUMA__
	Set(HEADER_GG_NEW_NOTICE, 				sizeof(TPacketGGNewNotice), 		"Worldboss");
#endif
#ifdef OFFLINE_SHOP
	Set(HEADER_GG_OFFLINE_SHOP_SEND_MESSAGE, sizeof(TPacketGGOfflineShopMessage), "OfflineShopUpdateMessage");
#endif
#ifdef ENABLE_DECORUM
	Set(HEADER_GG_DECORUM_ARENA_START,		sizeof(TPacketGGDecorumArenaStart),		"DecorumArenaStart");
	Set(HEADER_GG_DECORUM_RANDOM_REQUEST,	sizeof(TPacketGGDecorumArenaRequest),	"DecorumArenaRequest");
	Set(HEADER_GG_DECORUM_RANDOM_DELETE,	sizeof(TPacketGGDecorumArenaDelete),	"DecorumArenaDelete");
	Set(HEADER_GG_DECORUM_RANDOM_BROADCAST,	sizeof(TPacketGGDecorumArenaBroadcast),	"DecorumArenaBroadcast");
	Set(HEADER_GG_WARP_CHARACTER_ARENA,		sizeof(TPacketGGWarpCharacter),			"DecorumArenaWarpIN");
	
#endif
#ifdef ENABLE_SWITCHBOT
	Set(HEADER_GG_SWITCHBOT, sizeof(TPacketGGSwitchbot), "Switchbot");
#endif

#ifdef HANDSHAKE_FIX
	Set(HEADER_GG_HANDSHAKE_VALIDATION, sizeof(TPacketGGHandshakeValidate), "HandShakeValidation");
#endif
}

CPacketInfoGG::~CPacketInfoGG()
{
	Log("p2p_packet_info.txt");
}

