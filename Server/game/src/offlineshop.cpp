#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "mob_manager.h"
#include "locale_service.h"
#include "offlineshop.h"
#include "p2p.h"

#ifdef OFFLINE_SHOP
#include "offlineshop_manager.h"
#endif

COfflineShop::COfflineShop() : m_pkOfflineShopNPC(NULL), m_dwDisplayedCount(0), 
/*New Function*/
dwItems(0),
bIsClosing(false),
bIsManaging(false),
bAutoExpireStart(false)
/*New Function*/
{
	m_pGrid = M2_NEW CGrid(8, 8);
}

COfflineShop::~COfflineShop()
{
	TPacketGCShop pack;
	pack.header = HEADER_GC_OFFLINE_SHOP;
	pack.subheader = SHOP_SUBHEADER_GC_END;
	pack.size = sizeof(TPacketGCShop);

	Broadcast(&pack, sizeof(pack));

	for (GuestMapType::iterator it = m_map_guest.begin(); it != m_map_guest.end(); ++it)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(it->first);
	
		if (tch == NULL)
			continue;
	
		if (tch)
			tch->SetOfflineShop (NULL);
	}

	M2_DELETE(m_pGrid);
}

void COfflineShop::SetOfflineShopNPC(LPCHARACTER npc)
{
	m_pkOfflineShopNPC = npc;
}

void COfflineShop::SetShopItems(TShopItemTable * pTable, BYTE bItemCount, LPCHARACTER ch)
{
	if (!ch)
		return;

	m_pGrid->Clear();

	m_offlineShopItemVector.resize(OFFLINE_SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_offlineShopItemVector[0], 0, sizeof(OFFLINE_SHOP_ITEM) * m_offlineShopItemVector.size());

	for (int i = 0; i < bItemCount; ++i)
	{
		int iPos;
		LPITEM pkItem = ch->GetItem(pTable->pos);

		if (!pkItem){
			sys_err("COfflineShop::SetShopItems: cannot find item on pos (%d, %d) (name: %s)", pTable->pos.window_type, pTable->pos.cell, ch->GetName());
			continue;}

		if(pkItem->IsEquipped() == true || pkItem->isLocked() == true || pkItem->IsExchanging()){continue;}
#ifdef ENABLE_NEW_ITEM_LOCK_SYSTEM
		if (pkItem->GetLocks() > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Kilitli itemle bunu yapamazsin"));
			continue;
		}
#endif

		const TItemTable * item_table = pkItem->GetProto();

		if (!item_table){
			sys_err("COfflineShop::SetShopItems: no item table by item vnum #%d", pTable->vnum);
			continue;}

		if (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP)){continue;}

		iPos = pTable->display_pos;

		if (iPos < 0){
			sys_err("COfflineShop::SetShopItems: not enough shop window!");
			continue;}

		//sys_log(0, "COfflineShop::SetShopItems: use position %d", iPos);

		if (!m_pGrid->IsEmpty(iPos, 1, pkItem->GetSize())){
			sys_err("COfflineShop::SetShopItems: not empty position offline shop %s[%d]", ch->GetName(), ch->GetPlayerID());
			continue;}

		m_pGrid->Put(iPos, 1, pkItem->GetSize());

		++dwItems;

		/* memory part begin */
		OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[iPos];

		//////////
		// bIsSold already declared as = false in offlineshop.h
		offShopItem.owner_id = ch->GetPlayerID();
		offShopItem.pos = iPos;
		offShopItem.count = pkItem->GetCount();
		offShopItem.price = pTable->price;
		
		offShopItem.dwItemID = pkItem->GetID();
		
#ifdef ENABLE_CHEQUE_SYSTEM
		offShopItem.cheque = pTable->cheque_price;
#endif
		
		offShopItem.vnum = pkItem->GetVnum();

		for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
		{
			offShopItem.alSockets[x] = pkItem->GetSocket(x);
		}

		for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
		{
			offShopItem.aAttr[x].bType = pkItem->GetAttributeType(x);
			offShopItem.aAttr[x].sValue = pkItem->GetAttributeValue(x);
#ifdef __FROZENBONUS_SYSTEM__
			offShopItem.aAttr[x].isFrozen = pkItem->IsFrozenBonus(x);
#endif
		}

#ifdef CHANGELOOK_SYSTEM
		offShopItem.transmutation = pkItem->GetTransmutation();
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
		offShopItem.bItemType = pkItem->GetType();
		offShopItem.bItemSubType = pkItem->GetSubType();
		offShopItem.bItemLevel = pkItem->GetLevelLimit();
		offShopItem.bItemRefine = pkItem->GetRefineLevel();
		offShopItem.dwFlag = pkItem->GetFlag();
		offShopItem.dwAntiFlag = pkItem->GetAntiFlag();
		strncpy(offShopItem.szItemName, pkItem->GetName(), ITEM_NAME_MAX_LEN);
#endif
		/* memory part end */

		/* query part begin */
		char szColumns[QUERY_MAX_LEN], szValues[QUERY_MAX_LEN];

		snprintf(szColumns, sizeof(szColumns),
							"owner_id"
							", pos"
							", count"
							", price"
#ifdef ENABLE_CHEQUE_SYSTEM
							", cheque"
#endif
							", vnum"
							", socket0"
							", socket1"
							", socket2"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
							", socket3"
#endif
							", attrtype0"
							", attrvalue0"
							", attrtype1"
							", attrvalue1"
							", attrtype2"
							", attrvalue2"
							", attrtype3"
							", attrvalue3"
							", attrtype4"
							", attrvalue4"
							", attrtype5"
							", attrvalue5"
							", attrtype6"
							", attrvalue6"
#ifdef __FROZENBONUS_SYSTEM__
							", attrfrozen0, attrfrozen1,"
							"attrfrozen2, attrfrozen3,"
							"attrfrozen4, attrfrozen5,"
							"attrfrozen6"
#endif
#ifdef CHANGELOOK_SYSTEM
							", transmutation"
#endif
// New function
							", id_item"
		);

		snprintf(szValues, sizeof(szValues),
							"%u"
							", %d"
							", %u"
							// BEGIN_MAX_YANG
							", %d"
							// END_OF_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
							", %d"
#endif
							", %u"
							", %ld"
							", %ld"
							", %ld"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
							", %ld"
#endif
							
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
							", %d"
#ifdef __FROZENBONUS_SYSTEM__
							", %d, %d, %d, %d, %d, %d, %d"
#endif
#ifdef CHANGELOOK_SYSTEM
							", %d"
#endif
							", %d"
							, ch->GetPlayerID(),
							iPos,
							pkItem->GetCount(),
							pTable->price,
#ifdef ENABLE_CHEQUE_SYSTEM
							pTable->cheque_price,
#endif
							pkItem->GetVnum(),
							pkItem->GetSocket(0),
							pkItem->GetSocket(1),
							pkItem->GetSocket(2),
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
							pkItem->GetSocket(3),
#endif
							pkItem->GetAttributeType(0),
							pkItem->GetAttributeValue(0),
							pkItem->GetAttributeType(1),
							pkItem->GetAttributeValue(1),
							pkItem->GetAttributeType(2),
							pkItem->GetAttributeValue(2),
							pkItem->GetAttributeType(3),
							pkItem->GetAttributeValue(3),
							pkItem->GetAttributeType(4),
							pkItem->GetAttributeValue(4),
							pkItem->GetAttributeType(5),
							pkItem->GetAttributeValue(5),
							pkItem->GetAttributeType(6),
							pkItem->GetAttributeValue(6)
#ifdef __FROZENBONUS_SYSTEM__
							, pkItem->IsFrozenBonus(0),
							pkItem->IsFrozenBonus(1),
							pkItem->IsFrozenBonus(2),
							pkItem->IsFrozenBonus(3),
							pkItem->IsFrozenBonus(4),
							pkItem->IsFrozenBonus(5),
							pkItem->IsFrozenBonus(6)
#endif
#ifdef CHANGELOOK_SYSTEM
							, pkItem->GetTransmutation()
#endif
							,pkItem->GetID()
		);

		char szInsertQuery[QUERY_MAX_LEN];
		snprintf(szInsertQuery, sizeof(szInsertQuery), "INSERT INTO %soffline_shop_item (%s) VALUES (%s)", get_table_postfix(), szColumns, szValues);
		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szInsertQuery));
		/* query part end */

		/* log for web */
		LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pkItem->GetName(), "PUT");
		/* end log for web */

		ITEM_MANAGER::instance().RemoveItem(pkItem);

		++pTable;
	}
}

// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShop::PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice, int bCheque)
#else
void COfflineShop::PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice)
#endif
// END_OF_MAX_YANG
{
	if (!ch || !ch->CanHandleItem())
		return;
	
	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attendi per completare l'azione."));
		return;
	}
	
	// if (ch->IsHack())
		// return;
	
	ch->SetMyOfflineShopTime();

	LPITEM pkItem = ch->GetItem(item_pos);

	if (!pkItem)
		return;

	if (pkItem->IsEquipped() == true || pkItem->isLocked() == true || pkItem->IsExchanging())
		return;

#ifdef ENABLE_NEW_ITEM_LOCK_SYSTEM
	if (pkItem->GetLocks() > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Kilitli itemle bunu yapamazsin"));
		return;
	}
#endif

	const TItemTable * item_table = pkItem->GetProto();

	if (!item_table)
		return;

	if (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP))
		return;

	if (iPos < 0)
	{
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough space to put your item!"));
		return;
	}

	if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai abbastanza spazio per inserire il tuo item su: %d !"), iPos);
		return;
	}
	
	if (bIsClosing == true)
		return;

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
		return;
	}

	if (bIsManaging == true)
		return;

	bIsManaging = true;

	m_pGrid->Put(iPos, 1, item_table->bSize);

	++dwItems;

	/* memory part begin */
	OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[iPos];

	//////////
	// bIsSold already declared as = false in offlineshop.h
	offShopItem.owner_id = ch->GetPlayerID();
	offShopItem.pos = iPos;
	offShopItem.count = pkItem->GetCount();
	offShopItem.dwItemID = pkItem->GetID();
	
	// BEGIN_MAX_YANG
	offShopItem.price = llPrice;
	// END_OF_MAX_YANG
	
#ifdef ENABLE_CHEQUE_SYSTEM
	offShopItem.cheque = bCheque;
#endif
	
	offShopItem.vnum = pkItem->GetVnum();

	for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
	{
		offShopItem.alSockets[x] = pkItem->GetSocket(x);
	}

	for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
	{
		offShopItem.aAttr[x].bType = pkItem->GetAttributeType(x);
		offShopItem.aAttr[x].sValue = pkItem->GetAttributeValue(x);
#ifdef __FROZENBONUS_SYSTEM__
		offShopItem.aAttr[x].isFrozen = pkItem->IsFrozenBonus(x);
#endif
	}
	
#ifdef CHANGELOOK_SYSTEM
	offShopItem.transmutation = pkItem->GetTransmutation();
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
	offShopItem.bItemType = pkItem->GetType();
	offShopItem.bItemSubType = pkItem->GetSubType();
	offShopItem.bItemLevel = pkItem->GetLevelLimit();
	offShopItem.bItemRefine = pkItem->GetRefineLevel();
	offShopItem.dwFlag = pkItem->GetFlag();
	offShopItem.dwAntiFlag = pkItem->GetAntiFlag();
	strncpy(offShopItem.szItemName, pkItem->GetName(), ITEM_NAME_MAX_LEN);
#endif
	/* memory part end */

	/* query part begin */
	char szColumns[QUERY_MAX_LEN], szValues[QUERY_MAX_LEN];

	snprintf(szColumns, sizeof(szColumns),
						"owner_id"
						", pos"
						", count"
						", price"
						", vnum"
						", socket0"
						", socket1"
						", socket2"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						", socket3"
#endif
						", attrtype0"
						", attrvalue0"
						", attrtype1"
						", attrvalue1"
						", attrtype2"
						", attrvalue2"
						", attrtype3"
						", attrvalue3"
						", attrtype4"
						", attrvalue4"
						", attrtype5"
						", attrvalue5"
						", attrtype6"
						", attrvalue6"
#ifdef ENABLE_CHEQUE_SYSTEM
						", cheque"
#endif
#ifdef __FROZENBONUS_SYSTEM__
						", attrfrozen0, attrfrozen1,"
						"attrfrozen2, attrfrozen3,"
						"attrfrozen4, attrfrozen5,"
						"attrfrozen6"
#endif
#ifdef CHANGELOOK_SYSTEM
						", transmutation"
#endif
						", id_item"
	);

	snprintf(szValues, sizeof(szValues),
						"%u"
						", %d"
						", %u"
						// BEGIN_MAX_YANG
						", %d"
						// END_OF_MAX_YANG
						", %u"
						", %ld"
						", %ld"
						", %ld"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						", %ld"
#endif
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
#ifdef ENABLE_CHEQUE_SYSTEM
						", %d"
#endif
#ifdef __FROZENBONUS_SYSTEM__
						", %d, %d, %d, %d, %d, %d, %d"
#endif
#ifdef CHANGELOOK_SYSTEM
						", %d"
#endif
						", %d"
						, ch->GetPlayerID(),
						iPos,
						pkItem->GetCount(),
						// BEGIN_MAX_YANG
						llPrice,
						// END_OF_MAX_YANG
						pkItem->GetVnum(),
						pkItem->GetSocket(0),
						pkItem->GetSocket(1),
						pkItem->GetSocket(2),
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						pkItem->GetSocket(3),
#endif
						pkItem->GetAttributeType(0),
						pkItem->GetAttributeValue(0),
						pkItem->GetAttributeType(1),
						pkItem->GetAttributeValue(1),
						pkItem->GetAttributeType(2),
						pkItem->GetAttributeValue(2),
						pkItem->GetAttributeType(3),
						pkItem->GetAttributeValue(3),
						pkItem->GetAttributeType(4),
						pkItem->GetAttributeValue(4),
						pkItem->GetAttributeType(5),
						pkItem->GetAttributeValue(5),
						pkItem->GetAttributeType(6),
						pkItem->GetAttributeValue(6)
#ifdef ENABLE_CHEQUE_SYSTEM
						, bCheque
#endif
#ifdef __FROZENBONUS_SYSTEM__
						, pkItem->IsFrozenBonus(0),
						pkItem->IsFrozenBonus(1),
						pkItem->IsFrozenBonus(2),
						pkItem->IsFrozenBonus(3),
						pkItem->IsFrozenBonus(4),
						pkItem->IsFrozenBonus(5),
						pkItem->IsFrozenBonus(6)
#endif

						, pkItem->GetTransmutation()
						, pkItem->GetID()
			
	);

	char szInsertQuery[QUERY_MAX_LEN];
	snprintf(szInsertQuery, sizeof(szInsertQuery), "INSERT INTO %soffline_shop_item (%s) VALUES (%s)", get_table_postfix(), szColumns, szValues);
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szInsertQuery));
	/* query part end */

	/* log for web */
	LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pkItem->GetName(), "PUT_REMOTE");
	/* end log for web */

	// BroadcastUpdateItem(iPos);

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("L'oggetto %s e' stato aggiunto al negozio, Yang: %d e Won: %d."), pkItem->GetName(), llPrice, bCheque);
#else
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("L'oggetto %s e' stato aggiunto al negozio, Yang: %d."), pkItem->GetName(), llPrice);
#endif
	// END_OF_MAX_YANG
	
	ch->SetMyOfflineShopTime();

	ITEM_MANAGER::instance().RemoveItem(pkItem);
	
	COfflineShopManager::instance().SendItemsEditMode(ch);
	bIsManaging = false;
}


// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShop::PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice, int bCheque)
#else
void COfflineShop::PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice)
#endif
// END_OF_MAX_YANG
{
	if (!ch || !ch->CanHandleItem())
		return;
	
	if (ch->IsHack())
		return;

	LPITEM pkItem = ch->GetItem(item_pos);

	if (!pkItem)
		return;

	if (pkItem->IsEquipped() == true || pkItem->isLocked() == true || pkItem->IsExchanging())
		return;

#ifdef ENABLE_NEW_ITEM_LOCK_SYSTEM
	if (pkItem->GetLocks() > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Kilitli itemle bunu yapamazsin"));
		return;
	}
#endif

	const TItemTable * item_table = pkItem->GetProto();

	if (!item_table)
		return;

	if (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP))
		return;

	int iPos = m_pGrid->FindBlank(1, item_table->bSize);

	if (iPos < 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai abbastanza spazio per inserire l'oggetto."));
		return;
	}

	if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai abbastanza spazio per inserire l'oggetto."));
		return;
	}

	if (bIsClosing == true)
		return;

	if (bIsManaging == true)
		return;

	bIsManaging = false;

	m_pGrid->Put(iPos, 1, item_table->bSize);

	++dwItems;

	/* memory part begin */
	OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[iPos];

	//////////
	// bIsSold already declared as = false in offlineshop.h
	offShopItem.owner_id = ch->GetPlayerID();
	offShopItem.pos = iPos;
	offShopItem.count = pkItem->GetCount();
	// BEGIN_MAX_YANG
	offShopItem.price = llPrice;
	// END_OF_MAX_YANG
	
#ifdef ENABLE_CHEQUE_SYSTEM
	offShopItem.cheque = bCheque;
#endif
	
	offShopItem.vnum = pkItem->GetVnum();

	for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
	{
		offShopItem.alSockets[x] = pkItem->GetSocket(x);
	}

	for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
	{
		offShopItem.aAttr[x].bType = pkItem->GetAttributeType(x);
		offShopItem.aAttr[x].sValue = pkItem->GetAttributeValue(x);
#ifdef __FROZENBONUS_SYSTEM__
		offShopItem.aAttr[x].isFrozen = pkItem->IsFrozenBonus(x);
#endif
	}
	
#ifdef CHANGELOOK_SYSTEM
	offShopItem.transmutation = pkItem->GetTransmutation();
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
	offShopItem.bItemType = pkItem->GetType();
	offShopItem.bItemSubType = pkItem->GetSubType();
	offShopItem.bItemLevel = pkItem->GetLevelLimit();
	offShopItem.bItemRefine = pkItem->GetRefineLevel();
	offShopItem.dwFlag = pkItem->GetFlag();
	offShopItem.dwAntiFlag = pkItem->GetAntiFlag();
	strncpy(offShopItem.szItemName, pkItem->GetName(), ITEM_NAME_MAX_LEN);
#endif
	/* memory part end */

	/* query part begin */
	char szColumns[QUERY_MAX_LEN], szValues[QUERY_MAX_LEN];

	snprintf(szColumns, sizeof(szColumns),
						"owner_id"
						", pos"
						", count"
						", price"
						", vnum"
						", socket0"
						", socket1"
						", socket2"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						", socket3"
#endif
						", attrtype0"
						", attrvalue0"
						", attrtype1"
						", attrvalue1"
						", attrtype2"
						", attrvalue2"
						", attrtype3"
						", attrvalue3"
						", attrtype4"
						", attrvalue4"
						", attrtype5"
						", attrvalue5"
						", attrtype6"
						", attrvalue6"
#ifdef ENABLE_CHEQUE_SYSTEM
						", cheque"
#endif
#ifdef __FROZENBONUS_SYSTEM__
						", attrfrozen0, attrfrozen1,"
						"attrfrozen2, attrfrozen3,"
						"attrfrozen4, attrfrozen5,"
						"attrfrozen6"
#endif
#ifdef CHANGELOOK_SYSTEM
						", transmutation"
#endif
	);

	snprintf(szValues, sizeof(szValues),
						"%u"
						", %d"
						", %u"
						// BEGIN_MAX_YANG
						", %d"
						// END_OF_MAX_YANG
						", %u"
						", %ld"
						", %ld"
						", %ld"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						", %ld"
#endif
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
						", %d"
#ifdef ENABLE_CHEQUE_SYSTEM
						", %d"
#endif
#ifdef __FROZENBONUS_SYSTEM__
						", %d, %d, %d, %d, %d, %d, %d"
#endif
#ifdef CHANGELOOK_SYSTEM
						", %d"
#endif
						, ch->GetPlayerID(),
						iPos,
						pkItem->GetCount(),
						// BEGIN_MAX_YANG
						llPrice,
						// END_OF_MAX_YANG
						pkItem->GetVnum(),
						pkItem->GetSocket(0),
						pkItem->GetSocket(1),
						pkItem->GetSocket(2),
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
						pkItem->GetSocket(3),
#endif
						pkItem->GetAttributeType(0),
						pkItem->GetAttributeValue(0),
						pkItem->GetAttributeType(1),
						pkItem->GetAttributeValue(1),
						pkItem->GetAttributeType(2),
						pkItem->GetAttributeValue(2),
						pkItem->GetAttributeType(3),
						pkItem->GetAttributeValue(3),
						pkItem->GetAttributeType(4),
						pkItem->GetAttributeValue(4),
						pkItem->GetAttributeType(5),
						pkItem->GetAttributeValue(5),
						pkItem->GetAttributeType(6),
						pkItem->GetAttributeValue(6)
#ifdef ENABLE_CHEQUE_SYSTEM
						, bCheque
#endif
#ifdef __FROZENBONUS_SYSTEM__
						, pkItem->IsFrozenBonus(0),
						pkItem->IsFrozenBonus(1),
						pkItem->IsFrozenBonus(2),
						pkItem->IsFrozenBonus(3),
						pkItem->IsFrozenBonus(4),
						pkItem->IsFrozenBonus(5),
						pkItem->IsFrozenBonus(6)
#endif
	);

	char szInsertQuery[QUERY_MAX_LEN];
	snprintf(szInsertQuery, sizeof(szInsertQuery), "INSERT INTO %soffline_shop_item (%s) VALUES (%s)", get_table_postfix(), szColumns, szValues);
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szInsertQuery));
	/* query part end */

	/* log for web */
	LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pkItem->GetName(), "PUT_REMOTE");
	/* end log for web */

	// BroadcastUpdateItem(iPos);

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("L'oggetto %s e' stato aggiunto al negozio, Yang: %d e Won: %d."), pkItem->GetName(), llPrice, bCheque);
#else
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("L'oggetto %s e' stato aggiunto al negozio, Yang: %d."), pkItem->GetName(), llPrice);
#endif
	// END_OF_MAX_YANG
	
	ch->SetMyOfflineShopTime();

	ITEM_MANAGER::instance().RemoveItem(pkItem);
	
	bIsManaging = false;
}

void COfflineShop::PutItemsBoot(DWORD dwAccount, DWORD dwOwner)
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery),
		"SELECT "
		"pos"
		", count"
		", vnum"
		", socket0"
		", socket1"
		", socket2"
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
		", socket3"
#endif
#ifdef CHANGELOOK_SYSTEM
		", transmutation"
#endif
		", attrtype0"
		", attrvalue0"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen0"
#endif
		", attrtype1"
		", attrvalue1"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen1"
#endif
		", attrtype2"
		", attrvalue2"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen2"
#endif
		", attrtype3"
		", attrvalue3"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen3"
#endif
		", attrtype4"
		", attrvalue4"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen4"
#endif
		", attrtype5"
		", attrvalue5"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen5"
#endif
		", attrtype6"
		", attrvalue6"
#ifdef __FROZENBONUS_SYSTEM__
		", attrfrozen6"
#endif
		", price"
		", cheque"
		", id_item"

		" FROM %soffline_shop_item WHERE owner_id = %u", get_table_postfix(), dwOwner);
	
	std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));

	// if shop is empty close
	if (pMsg->Get()->uiNumRows == 0)
	{
		COfflineShopManager::instance().AutoCloseOfflineShopBoot(dwAccount, dwOwner);
		return;
	}

	MYSQL_ROW row;

	m_pGrid->Clear();

	m_offlineShopItemVector.resize(OFFLINE_SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_offlineShopItemVector[0], 0, sizeof(OFFLINE_SHOP_ITEM) * m_offlineShopItemVector.size());

	while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		int cur = 0;
		DWORD dwPos = atoi(row[cur++]);

		OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[dwPos];

		offShopItem.pos = dwPos;
		offShopItem.count = atoi(row[cur++]);
		DWORD dwVnum = atoi(row[cur++]);
		offShopItem.vnum = dwVnum;
		
		for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
			offShopItem.alSockets[x] = atoi(row[cur++]);
		
	#ifdef CHANGELOOK_SYSTEM
		offShopItem.transmutation = atoi(row[cur++]);
	#endif
	
		for (int i = 0, iStartAttributeType = 7, iStartAttributeValue = iStartAttributeType + 1, iStartFrozenValue = iStartAttributeValue + 1; i < ITEM_ATTRIBUTE_MAX_NUM; ++i, iStartAttributeType += 3, iStartAttributeValue += 3, iStartFrozenValue += 3)
		{
			offShopItem.aAttr[i].bType = atoi(row[cur++]);
			offShopItem.aAttr[i].sValue = atoi(row[cur++]);
			#ifdef __FROZENBONUS_SYSTEM__
			offShopItem.aAttr[i].isFrozen = atoi(row[cur++]);
			#endif
		}
		
		offShopItem.price = atoi(row[cur++]);
#ifdef ENABLE_CHEQUE_SYSTEM
		offShopItem.cheque = atoi(row[cur++]);
#endif

		offShopItem.dwItemID = atoi(row[cur++]);

		const TItemTable* proto = ITEM_MANAGER::instance().GetTable(dwVnum);
		
		if (!proto)
			continue;

		if (dwPos < 0){
			sys_err("COfflineShop::BootItems: not enough shop window!");
			continue;}


		if (!m_pGrid->IsEmpty(dwPos, 1, proto->bSize)){
			sys_err("COfflineShop::BootItems: not empty position offline pid dwOwner [%d] from pos :%d", dwOwner, dwPos);
			continue;}

		m_pGrid->Put(dwPos, 1, proto->bSize);
	
#ifdef PRIVATESHOP_SEARCH_SYSTEM
		offShopItem.bItemType = proto->bType;
		offShopItem.bItemSubType = proto->bSubType;
		
		for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
		{
			if (proto->aLimits[i].bType == LIMIT_LEVEL)
			{
				offShopItem.bItemLevel = proto->aLimits[i].lValue;
			}
		}

		offShopItem.bItemRefine = proto->dwRefinedVnum;
		offShopItem.dwFlag = proto->dwFlags;
		offShopItem.dwAntiFlag = proto->dwAntiFlags;
		strncpy(offShopItem.szItemName, proto->szLocaleName[0], ITEM_NAME_MAX_LEN);
#endif

		offShopItem.owner_id = dwOwner;
		
		++dwItems;

	}

}

// FUNCTIONS_EXTRA_BEGIN
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShop::EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice, int bCheque)
#else
void COfflineShop::EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice)
#endif
{
	if (!ch || llPrice <= 0 || llPrice > GOLD_MAX)
		return;

#ifdef ENABLE_CHEQUE_SYSTEM
	if (bCheque > CHEQUE_MAX)
		return;
#endif

	// if (!ch->CanHandleItem())
		// return;
	
	// if (ch->IsHack())
		// return;	
	
	// Remove Guest While you edit price
	// RemoveAllGuestException();

	OFFLINE_SHOP_ITEM& r_item = m_offlineShopItemVector[item_pos];

	// Check Sold Status, to prevent directquery
	if (r_item.bIsSold == true || r_item.vnum <= 0 || bIsClosing == true)
		return;
	// Check Sold Status, to prevent directquery

	if (bIsManaging == true)
		return;

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
		return;
	}

	bIsManaging = true;

	r_item.price = llPrice;
#ifdef ENABLE_CHEQUE_SYSTEM
	r_item.cheque = bCheque;
#endif

	// WARNING, IS BETTER USING UNIQUE_PTR
	#ifdef ENABLE_CHEQUE_SYSTEM
		SQLMsg* p = DBManager::instance().DirectQuery("UPDATE %soffline_shop_item SET price = %d, cheque = %d WHERE owner_id = %u AND vnum = %d AND id_item = %d ",get_table_postfix(), llPrice, bCheque, r_item.owner_id, r_item.vnum, r_item.dwItemID);
	#else
		SQLMsg* p = DBManager::instance().DirectQuery("UPDATE %soffline_shop_item SET price = %d WHERE owner_id = %u AND vnum = %d AND id_item = %d",get_table_postfix(), llPrice, r_item.owner_id, r_item.vnum, r_item.dwItemID);
	#endif
	if (p)
	{
		delete p;
		p = NULL;
	}
	// WARNING, IS BETTER USING UNIQUE_PTR
	
	// BroadcastUpdateItem(item_pos);
	
	bIsManaging = false;
	
// #ifdef ENABLE_CHEQUE_SYSTEM
	// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item %s has been successfully edit to your offline shop, price: %d and cheque: %d."), item->GetName(), llPrice, bCheque);
// #else
	// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item %s has been successfully edit to your offline shop, price: %d."), item->GetName(), llPrice);
// #endif
}

void COfflineShop::DelSingleItem(LPCHARACTER ch, int item_pos)
{
	if (!ch || !ch->CanHandleItem())
		return;
	
	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(5))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attendi per completare l'azione."));
		return;
	}
	
	// if (ch->IsHack())
		// return;
	
	if (bIsManaging == true)
		return;
	
	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
		return;
	}
	
	ch->SetMyOfflineShopTime();

	// Remove Guest To Prevent Copy Item
	RemoveAllGuestException();

	OFFLINE_SHOP_ITEM& r_item = m_offlineShopItemVector[item_pos];

	// Check Sold Status, to prevent copy item || Shop trying to close
	if (r_item.bIsSold == true || bIsClosing == true || r_item.vnum <= 0)
		return;
	// Check Sold Status, to prevent copy item || Shop trying to close

	LPITEM item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return;
	
	bIsManaging = true;

	// if (m_pGrid->IsEmpty(item_pos, 1, item->GetSize()))
	// {
		// M2_DESTROY_ITEM(item); // delete item
		// r_item.vnum = 0;
		// return;
	// }

	int iEmptyPos;

	if (item->IsDragonSoul())
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsBook())
		iEmptyPos = ch->GetEmptyBookInventory(item);
	else if (item->IsUpgradeItem())
		iEmptyPos = ch->GetEmptyUpgradeInventory(item);
	else if (item->IsStone())
		iEmptyPos = ch->GetEmptyStoneInventory(item);
#endif
	else
		iEmptyPos = ch->GetEmptyInventory(item->GetSize());

	if (iEmptyPos < 0)
	{
		M2_DESTROY_ITEM(item); // delete item
		bIsManaging = false;
		ch->ChatPacket(CHAT_TYPE_INFO, "Inventario pieno.");
		return;
	}

	item->SetSockets(r_item.alSockets);
	item->SetAttributes(r_item.aAttr);
	
#ifdef CHANGELOOK_SYSTEM
	item->SetTransmutation(r_item.transmutation);
#endif

	if (item->IsDragonSoul())
		item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsBook())
		item->AddToCharacter(ch, TItemPos(BOOK_INVENTORY, iEmptyPos));
	else if (item->IsUpgradeItem())
		item->AddToCharacter(ch, TItemPos(UPGRADE_INVENTORY, iEmptyPos));
	else if (item->IsStone())
		item->AddToCharacter(ch, TItemPos(STONE_INVENTORY, iEmptyPos));
#endif
	else
		item->AddToCharacter(ch, TItemPos(INVENTORY,iEmptyPos));

	// WARNING, IS BETTER USING UNIQUE_PTR
	SQLMsg* p = DBManager::instance().DirectQuery("DELETE FROM %soffline_shop_item WHERE owner_id = %u AND pos = %d AND vnum = %d LIMIT 1", get_table_postfix(), r_item.owner_id, item_pos, r_item.vnum);
	if (p)
		delete p;
	// WARNING, IS BETTER USING UNIQUE_PTR

	r_item.vnum = 0;
	r_item.count = 0;
	
	m_pGrid->Get(item_pos, 1, item->GetSize());
	
	--dwItems;

	// BroadcastUpdateItem(item_pos);
	bIsManaging = false;
	
	if (dwItems <= 0)
	{
		// RemoveAllGuest();
		COfflineShopManager::instance().AutoCloseOfflineShop(ch);
	}
	
}

bool COfflineShop::SendInfoItems(LPCHARACTER ch, LPCHARACTER npc)
{
	TPacketGCShop pack;
	pack.header = HEADER_GC_OFFLINE_SHOP;
	pack.subheader = SHOP_SUBHEADER_GC_OFFLINE_SHOP_INFO;

	TPacketGCOfflineShopStart pack2;
	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = npc->GetVID();
	
	pack2.dwRemainTime = npc->GetOfflineShopTimer();
	pack2.map = npc->GetMapIndex();
	pack2.x = npc->GetX();
	pack2.y = npc->GetY();

	for (DWORD i = 0; i < m_offlineShopItemVector.size() && i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[i];

		pack2.items[i].count = offShopItem.count;
		pack2.items[i].price = offShopItem.price;
#ifdef ENABLE_CHEQUE_SYSTEM
		pack2.items[i].cheque = offShopItem.cheque;
#endif
		pack2.items[i].vnum = offShopItem.vnum;
		pack2.items[i].bIsSold = offShopItem.bIsSold;
		strncpy(pack2.items[i].buyerName, offShopItem.buyerName, CHARACTER_NAME_MAX_LEN);

		for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
		{
			pack2.items[i].alSockets[x] = offShopItem.alSockets[x];
		}

		for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
		{
			const TPlayerItemAttribute & c_rItemAttr = offShopItem.aAttr[x];

			pack2.items[i].aAttr[x].bType = c_rItemAttr.bType;
			pack2.items[i].aAttr[x].sValue = c_rItemAttr.sValue;
#ifdef __FROZENBONUS_SYSTEM__
			pack2.items[i].aAttr[x].isFrozen = c_rItemAttr.isFrozen;
#endif
		}

#ifdef CHANGELOOK_SYSTEM
		pack2.items[i].transmutation = offShopItem.transmutation;
#endif
	}

	pack.size = sizeof(pack)+sizeof(pack2);
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCOfflineShopStart));

	return true;
}

//FUNCTIONS_EXTRA_END

bool COfflineShop::AddGuest(LPCHARACTER ch, LPCHARACTER npc)
{
	if (!ch || ch->GetExchange() || ch->GetShop() || ch->GetMyShop() || ch->GetOfflineShop())
		return false;

	ch->SetOfflineShop(this);
	m_map_guest.insert(GuestMapType::value_type(ch->GetPlayerID(), false));

	TPacketGCShop pack;
	pack.header = HEADER_GC_OFFLINE_SHOP;
	pack.subheader = SHOP_SUBHEADER_GC_START;

	TPacketGCOfflineShopStart pack2;
	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = npc->GetVID();
	
	pack2.dwRemainTime = npc->GetOfflineShopTimer();

	++m_dwDisplayedCount;

	pack2.m_dwDisplayedCount = m_dwDisplayedCount;

	for (DWORD i = 0; i < m_offlineShopItemVector.size() && i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const OFFLINE_SHOP_ITEM & offShopItem = m_offlineShopItemVector[i];

		pack2.items[i].count = offShopItem.count;
		pack2.items[i].price = offShopItem.price;
#ifdef ENABLE_CHEQUE_SYSTEM
		pack2.items[i].cheque = offShopItem.cheque;
#endif
		pack2.items[i].vnum = offShopItem.vnum;
		pack2.items[i].bIsSold = offShopItem.bIsSold;
		strncpy(pack2.items[i].buyerName, offShopItem.buyerName, CHARACTER_NAME_MAX_LEN);

		for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
		{
			pack2.items[i].alSockets[x] = offShopItem.alSockets[x];
		}

		for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
		{
			const TPlayerItemAttribute & c_rItemAttr = offShopItem.aAttr[x];

			pack2.items[i].aAttr[x].bType = c_rItemAttr.bType;
			pack2.items[i].aAttr[x].sValue = c_rItemAttr.sValue;
#ifdef __FROZENBONUS_SYSTEM__
			pack2.items[i].aAttr[x].isFrozen = c_rItemAttr.isFrozen;
#endif
		}

#ifdef CHANGELOOK_SYSTEM
		pack2.items[i].transmutation = offShopItem.transmutation;
#endif
	}

	pack.size = sizeof(pack)+sizeof(pack2);
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCOfflineShopStart));

	return true;
}

void COfflineShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetOfflineShop() != this)
		return;

	GuestMapType::iterator it;

	it = m_map_guest.find(ch->GetPlayerID());
	
	if (it == m_map_guest.end())
		return;

	m_map_guest.erase(ch->GetPlayerID());

	ch->SetOfflineShop(NULL);

	TPacketGCShop pack;
	pack.header = HEADER_GC_OFFLINE_SHOP;
	pack.subheader = SHOP_SUBHEADER_GC_END;
	pack.size = sizeof(TPacketGCShop);

	ch->GetDesc()->Packet(&pack, sizeof(pack));
}

void COfflineShop::RemoveAllGuest()
{
	GuestMapType::iterator it = m_map_guest.begin();
	while (it != m_map_guest.end())
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(it->first);
	
		if (tch == NULL)
			continue;
	
		if (tch)
			tch->SetOfflineShop (NULL);
		
		if (tch)
		{
			tch->SetShop(NULL);

			TPacketGCShop pack;

			pack.header = HEADER_GC_OFFLINE_SHOP;
			pack.subheader = SHOP_SUBHEADER_GC_END;
			pack.size = sizeof(TPacketGCShop);
			
			if (tch->GetDesc())
				tch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		m_map_guest.erase(it->first);
		
		it++;
	}
}
		// if (ch->GetPlayerID() == m_pkOfflineShopNPC->GetOfflineShopRealOwner())

void COfflineShop::RemoveAllGuestException()
{
	GuestMapType::iterator it = m_map_guest.begin();
	while (it != m_map_guest.end())
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(it->first);
	
		if (tch == NULL)
			continue;
	
		if (tch)
			tch->SetOfflineShop (NULL);
		
		if (tch)
		{
			tch->SetShop(NULL);

			TPacketGCShop pack;

			pack.header = HEADER_GC_OFFLINE_SHOP;
			pack.subheader = SHOP_SUBHEADER_GC_END;
			pack.size = sizeof(TPacketGCShop);
			
			if (tch->GetDesc())
				tch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		m_map_guest.erase(it->first);
		
		it++;
	}
}

bool COfflineShop::CanSaveItems(LPCHARACTER ch)
{
	std::vector<OFFLINE_SHOP_ITEM>::iterator begin, end;
	begin = m_offlineShopItemVector.begin();
	end = m_offlineShopItemVector.end();

	size_t nTotalCount = 0;

	for (; begin != end; begin++)
	{
		if (!(*begin).vnum)
			continue;

		nTotalCount += (*begin).count;
	}

	return nTotalCount <= ch->CountEmptyInventory();
}

void COfflineShop::Destroy(LPCHARACTER npc)
{
	RemoveAllGuest();
	M2_DESTROY_CHARACTER(npc);
}

void COfflineShop::GiveBackMyItems(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->IsHack())
		return;

	// Close shop to prevent Duplicate item
	bIsClosing = true;
	// Close shop to prevent Duplicate item

	// RemoveAllGuest();

	/* cleanup begin */
	SQLMsg* p = DBManager::instance().DirectQuery("DELETE FROM %soffline_shop_item WHERE owner_id = %u", get_table_postfix(), ch->GetPlayerID());
	if (p)
		delete p;

	/* cleanup end */

	for (DWORD i = 0; i < m_offlineShopItemVector.size() && i < OFFLINE_SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const OFFLINE_SHOP_ITEM & r_item = m_offlineShopItemVector[i];

		if (r_item.bIsSold || !r_item.vnum)
			continue;

		LPITEM pItem = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

		if (pItem)
		{
			pItem->SetSockets(r_item.alSockets);
			pItem->SetAttributes(r_item.aAttr);

#ifdef CHANGELOOK_SYSTEM
			pItem->SetTransmutation(r_item.transmutation);
#endif

			int cell;

			// BEGIN_FOR_DRAGON_SOUL
			if (pItem->IsDragonSoul())
				cell = ch->GetEmptyDragonSoulInventory(pItem);

			else
				cell = ch->GetEmptyInventory(pItem->GetSize());
			// END_OF_FOR_DRAGON_SOUL

			if (cell != -1)
			{
				// BEGIN_FOR_DRAGON_SOUL
				if (pItem->IsDragonSoul())
					pItem->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, cell));

				else
					pItem->AddToCharacter(ch, TItemPos(INVENTORY,cell));
				// END_OF_FOR_DRAGON_SOUL
			}
			else
			{
				pItem->AddToGround(ch->GetMapIndex(), ch->GetXYZ());
				pItem->StartDestroyEvent();
				pItem->SetOwnership(ch, g_iOfflineShopOwnerShipTime);
			}

			/* log for web */
			LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pItem->GetName(), "GIVE_BACK");
			/* end log for web */
		}
	}
	
	ch->SetMyOfflineShopTime();
}

// BEGIN_MAX_YANG
#ifdef PRIVATESHOP_SEARCH_SYSTEM
int	COfflineShop::Buy(LPCHARACTER ch, BYTE bPos, bool bSearch /*bSearch = false*/)
#else
int	COfflineShop::Buy(LPCHARACTER ch, BYTE bPos)
#endif
// END_OF_MAX_YANG
{
	if (!ch)
		return SHOP_SUBHEADER_GC_END;
/*
	if (ch->IsHack())
		return false;
*/
	if (!ch->GetOfflineShopOwner())
		return SHOP_SUBHEADER_GC_END;

	if (ch->GetOfflineShopOwner()->GetOfflineShopRealOwner() == ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non puoi comprare oggetti dal tuo negozio."));
		return SHOP_SUBHEADER_GC_OK;
	}

	if (bPos >= m_offlineShopItemVector.size())
	{
		sys_log(0, "COfflineShop::Buy: invalid position %d : %s", bPos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}
	
	if (bIsManaging == true)
		return SHOP_SUBHEADER_GC_END;

	//sys_log(0, "COfflineShop::Buy: name: %s, pos: %d", ch->GetName(), bPos);

#ifdef PRIVATESHOP_SEARCH_SYSTEM
	if (!bSearch)
	{
		GuestMapType::iterator it = m_map_guest.find(ch->GetPlayerID());
		if (it == m_map_guest.end())
			return SHOP_SUBHEADER_GC_END;
	}
#else
	GuestMapType::iterator it = m_map_guest.find(ch->GetPlayerID());
	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;
#endif

	OFFLINE_SHOP_ITEM & r_item = m_offlineShopItemVector[bPos];

	if (r_item.bIsSold)
		return SHOP_SUBHEADER_GC_SOLD_OUT;

	// BEGIN_MAX_YANG
	int llPrice = r_item.price;
	// END_OF_MAX_YANG

	if (r_item.price < 0)
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;

	// BEGIN_MAX_YANG
	if (ch->GetGold() < static_cast<int>(llPrice))
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	// END_OF_MAX_YANG
	
#ifdef ENABLE_CHEQUE_SYSTEM
	int bCheque = r_item.cheque;
	
	if (ch->GetCheque() < (int)(bCheque))
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
#endif

	LPITEM item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return SHOP_SUBHEADER_GC_END;

	if (bIsManaging == true)
		return SHOP_SUBHEADER_GC_END;

	int iEmptyPos;

	// BEGIN_FOR_DRAGON_SOUL
	if (item->IsDragonSoul())
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsBook())
		iEmptyPos = ch->GetEmptyBookInventory(item);
	else if (item->IsUpgradeItem())
		iEmptyPos = ch->GetEmptyUpgradeInventory(item);
	else if (item->IsStone())
		iEmptyPos = ch->GetEmptyStoneInventory(item);
#endif
	else
		iEmptyPos = ch->GetEmptyInventory(item->GetSize());
	// END_OF_FOR_DRAGON_SOUL

	if (iEmptyPos < 0)
	{
		M2_DESTROY_ITEM(item);
		return SHOP_SUBHEADER_GC_INVENTORY_FULL;
	}

	// BEGIN_MAX_YANG
	ch->PointChange(POINT_GOLD, -static_cast<int>(llPrice), false);
	// END_OF_MAX_YANG
	
#ifdef ENABLE_CHEQUE_SYSTEM
	if (bCheque)
		ch->PointChange(POINT_CHEQUE, -bCheque);
#endif
	
	r_item.bIsSold = true;
	strncpy(r_item.buyerName, ch->GetName(), CHARACTER_NAME_MAX_LEN);

	item->SetSockets(r_item.alSockets);
	item->SetAttributes(r_item.aAttr);
	
#ifdef CHANGELOOK_SYSTEM
	item->SetTransmutation(r_item.transmutation);
#endif

	// BEGIN_FOR_DRAGON_SOUL
	if (item->IsDragonSoul())
		item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsBook())
		item->AddToCharacter(ch, TItemPos(BOOK_INVENTORY, iEmptyPos));
	else if (item->IsUpgradeItem())
		item->AddToCharacter(ch, TItemPos(UPGRADE_INVENTORY, iEmptyPos));
	else if (item->IsStone())
		item->AddToCharacter(ch, TItemPos(STONE_INVENTORY, iEmptyPos));
#endif
	else
		item->AddToCharacter(ch, TItemPos(INVENTORY,iEmptyPos));
	// END_OF_FOR_DRAGON_SOUL

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	SQLMsg* p = DBManager::instance().DirectQuery("UPDATE player.player SET gold_offlineshop = gold_offlineshop + %d, cheque_offlineshop = cheque_offlineshop + %d WHERE id = %u", llPrice, bCheque, ch->GetOfflineShopOwner()->GetOfflineShopRealOwner());
#else
	SQLMsg* p = DBManager::instance().DirectQuery("UPDATE player.player SET gold_offlineshop = gold_offlineshop + %d WHERE id = %u", llPrice, ch->GetOfflineShopOwner()->GetOfflineShopRealOwner());
#endif
	if (p)
	{
		delete p;
		p = NULL;
	}

	// END_OF_MAX_YANG
	p = DBManager::instance().DirectQuery("DELETE FROM %soffline_shop_item WHERE owner_id = %u AND pos = %d AND vnum = %d LIMIT 1", get_table_postfix(), r_item.owner_id, bPos, r_item.vnum);
	if (p)
		delete p;

	LogManager::instance().OfflineShopLog(ch->GetOfflineShopOwner()->GetOfflineShopRealOwnerAccountID(), item->GetName(), "SELL");

	BroadcastUpdateItem(bPos);

	ch->SetMyOfflineShopTime();
	ch->Save();

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(ch->GetOfflineShopOwner()->GetOfflineShopRealOwner());

	if (tch)
	{
		char msg[CHAT_MAX_LEN + 1];
		snprintf(msg, sizeof(msg), LC_TEXT("Il tuo item %s e' stato venduto, acquirente: %s."), item->GetName(), ch->GetName());

		LPDESC pkVictimDesc = tch->GetDesc();

		if (pkVictimDesc)
		{
			TPacketGCWhisper pack;

			int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

			pack.bHeader = HEADER_GC_WHISPER;
			pack.wSize = sizeof(TPacketGCWhisper) + len;
			pack.bType = WHISPER_TYPE_SYSTEM;
			strlcpy(pack.szNameFrom, "[Assistente]", sizeof(pack.szNameFrom));

			TEMP_BUFFER buf;

			buf.write(&pack, sizeof(TPacketGCWhisper));
			buf.write(msg, len);

			pkVictimDesc->Packet(buf.read_peek(), buf.size());
		}

		//tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Market Tezgâhýndan nesne satýldý: %s."), item->GetName());
	}
	else // P2P_FIND
	{
		TPacketGGOfflineShopMessage p;
		p.bHeader = HEADER_GG_OFFLINE_SHOP_SEND_MESSAGE;
		p.dwTargetPID = ch->GetOfflineShopOwner()->GetOfflineShopRealOwner();
		strlcpy(p.szItemName, item->GetName(), sizeof(p.szItemName));
		strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGOfflineShopMessage));
	}

	return (SHOP_SUBHEADER_GC_OK);
}

void COfflineShop::BroadcastUpdateItem(BYTE bPos)
{
	TPacketGCShop pack;
	TPacketGCShopUpdateOfflineItem pack2;

	TEMP_BUFFER buf;

	pack.header = HEADER_GC_OFFLINE_SHOP;
	pack.subheader = SHOP_SUBHEADER_GC_UPDATE_ITEM;
	pack.size = sizeof(pack) + sizeof(pack2);

	OFFLINE_SHOP_ITEM & r_item = m_offlineShopItemVector[bPos];

	pack2.item.count = r_item.count;
	pack2.item.price = r_item.price;
	
#ifdef ENABLE_CHEQUE_SYSTEM
	pack2.item.cheque = r_item.cheque;
#endif
	
	pack2.item.vnum = r_item.vnum;
	pack2.item.bIsSold = r_item.bIsSold;
	pack2.pos = r_item.pos;
	strncpy(pack2.item.buyerName, r_item.buyerName, CHARACTER_NAME_MAX_LEN);

	for (int x = 0; x < ITEM_SOCKET_MAX_NUM; x++)
	{
		pack2.item.alSockets[x] = r_item.alSockets[x];
	}

	for (int x = 0; x < ITEM_ATTRIBUTE_MAX_NUM; x++)
	{
		const TPlayerItemAttribute & c_rItemAttr = r_item.aAttr[x];

		pack2.item.aAttr[x].bType = c_rItemAttr.bType;
		pack2.item.aAttr[x].sValue = c_rItemAttr.sValue;
#ifdef __FROZENBONUS_SYSTEM__
		pack2.item.aAttr[x].isFrozen = c_rItemAttr.isFrozen;
#endif
	}

#ifdef CHANGELOOK_SYSTEM
	pack2.item.transmutation = r_item.transmutation;
#endif

	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));
	Broadcast(buf.read_peek(), buf.size());
}

void COfflineShop::Broadcast(const void * data, int bytes)
{
	for (GuestMapType::iterator it = m_map_guest.begin(); it != m_map_guest.end(); ++it)
	{
//
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(it->first);
	
		if (tch == NULL)
			continue;
	
		if (tch)
		{
			if (tch->GetDesc())
				tch->GetDesc()->Packet (data, bytes);
		}
//
		
	}
}


