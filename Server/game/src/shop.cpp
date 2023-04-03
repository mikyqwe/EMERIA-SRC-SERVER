#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
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
#include "monarch.h"
#include "mob_manager.h"
#include "locale_service.h"

//#define ENABLE_SHOP_BLACKLIST
/* ------------------------------------------------------------------------------------ */
CShop::CShop()
	: m_dwVnum(0), m_dwNPCVnum(0), m_pkPC(NULL)
{
	m_pGrid = M2_NEW CGrid(5, 8);
}

CShop::~CShop()
{
	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	Broadcast(&pack, sizeof(pack));

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		ch->SetShop(NULL);
		++it;
	}

	M2_DELETE(m_pGrid);
}

void CShop::SetPCShop(LPCHARACTER ch)
{
	m_pkPC = ch;
}

bool CShop::Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pTable)
{
	/*
	   if (NULL == CMobManager::instance().Get(dwNPCVnum))
	   {
	   sys_err("No such a npc by vnum %d", dwNPCVnum);
	   return false;
	   }
	 */
	sys_log(0, "SHOP #%d (Shopkeeper %d)", dwVnum, dwNPCVnum);

	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;

	short bItemCount;

	for (bItemCount = 0; bItemCount < SHOP_HOST_ITEM_MAX_NUM; ++bItemCount)
		if (0 == (pTable + bItemCount)->vnum)
			break;

	SetShopItems(pTable, bItemCount);
	return true;
}

void CShop::SetShopItems(TShopItemTable * pTable, short bItemCount)
{
	if (bItemCount > SHOP_HOST_ITEM_MAX_NUM)
		return;

	m_pGrid->Clear();

	m_itemVector.resize(SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_itemVector[0], 0, sizeof(SHOP_ITEM) * m_itemVector.size());

	for (int i = 0; i < bItemCount; ++i)
	{
		LPITEM pkItem = NULL;
		const TItemTable * item_table;

		if (m_pkPC)
		{
			pkItem = m_pkPC->GetItem(pTable->pos);

			if (!pkItem)
			{
				sys_err("cannot find item on pos (%d, %d) (name: %s)", pTable->pos.window_type, pTable->pos.cell, m_pkPC->GetName());
				continue;
			}

			item_table = pkItem->GetProto();
		}
		else
		{
			if (!pTable->vnum)
				continue;

			item_table = ITEM_MANAGER::instance().GetTable(pTable->vnum);
		}

		if (!item_table)
		{
			sys_err("Shop: no item table by item vnum #%d", pTable->vnum);
			continue;
		}

		int iPos;

		if (IsPCShop())
		{
			sys_log(0, "MyShop: use position %d", pTable->display_pos);
			iPos = pTable->display_pos;
		}
		// else
			// iPos = m_pGrid->FindBlank(1, item_table->bSize);
		//fix yang = 0
		else
		{
			if(pTable->price == 0 && pkItem && pkItem->GetGold() > 0)
			{
				pTable->price = pkItem->GetGold()*pTable->count;
			}
			iPos = m_pGrid->FindBlank(1, item_table->bSize);
		}
		///end fix

		if (iPos < 0)
		{
			sys_err("not enough shop window");
			continue;
		}

		if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
		{
			if (IsPCShop())
			{
				sys_err("not empty position for pc shop %s[%d]", m_pkPC->GetName(), m_pkPC->GetPlayerID());
			}
			else
			{
				sys_err("not empty position for npc shop");
			}
			continue;
		}

		m_pGrid->Put(iPos, 1, item_table->bSize);

		SHOP_ITEM & item = m_itemVector[iPos];

		item.pkItem = pkItem;
		item.itemid = 0;

		if (item.pkItem)
		{
			item.vnum = pkItem->GetVnum();
			item.count = pkItem->GetCount(); // PC 샵의 경우 아이템 개수는 진짜 아이템의 개수여야 한다.
			item.price = pTable->price; // 가격도 사용자가 정한대로..
#ifdef ENABLE_CHEQUE_SYSTEM
			item.cheque_price = pTable->cheque_price;
#endif
			item.itemid	= pkItem->GetID();
		}
		else
		{
			item.vnum = pTable->vnum;
			item.count = pTable->count;
	#ifdef ENABLE_MULTISHOP
			item.wPriceVnum = pTable->wPriceVnum;
			item.wPrice = pTable->wPrice;
#endif		
			if (IS_SET(item_table->dwFlags, ITEM_FLAG_COUNT_PER_1GOLD))
			{
				if (item_table->dwGold == 0)
					item.price = item.count;
				else
					item.price = item.count / item_table->dwGold;
			}
			else
				item.price = item_table->dwGold * item.count;
		}

		char name[36];
		snprintf(name, sizeof(name), "%-20s(#%-5d) (x %d)", item_table->szName, (int) item.vnum, item.count);

		sys_log(0, "SHOP_ITEM: %-36s PRICE %-5d", name, item.price);
		++pTable;
	}
}



extern bool FN_check_item_socket(LPITEM item);
int CShop::Buy(LPCHARACTER ch, BYTE pos)
{
	if (pos >= m_itemVector.size())
	{
		sys_log(0, "Shop::Buy : invalid position %d : %s", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	sys_log(0, "Shop::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;
	if (ch == m_pkPC)
		return SHOP_SUBHEADER_GC_END;
	SHOP_ITEM& r_item = m_itemVector[pos];

#ifdef ENABLE_CHEQUE_SYSTEM
	if (r_item.price < 0 && r_item.cheque_price <= 0)
#else
	if (r_item.price < 0)
#endif
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	LPITEM pkSelectedItem = ITEM_MANAGER::instance().Find(r_item.itemid);
#ifdef ENABLE_MULTISHOP
	DWORD dwWItemVnum = r_item.wPriceVnum;
	DWORD dwWItemPrice = r_item.wPrice;
#endif



	if (IsPCShop())
	 {
		
			 if (!pkSelectedItem)
			 {
				 sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item id %d : BuyerPID:%d SellerPID:%d",
						 r_item.itemid,
						 ch->GetPlayerID(),
						 m_pkPC->GetPlayerID());

				 return false;
			 }

			 if ((pkSelectedItem->GetOwner() != m_pkPC))
			{
				 sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item owner: BuyerPID:%d SellerPID:%d",
						 ch->GetPlayerID(),
						 m_pkPC->GetPlayerID());

				 return false;
			 }
		
	 }

	long long  dwPrice = r_item.price;
#ifdef ENABLE_CHEQUE_SYSTEM
	int byChequePrice = r_item.cheque_price;
#endif

#ifdef ENABLE_MULTISHOP
	if (dwWItemVnum > 0)
	{
		if (ch->CountSpecifyItem(dwWItemVnum) < (int)dwWItemPrice)
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM;
	}
	else if (ch->GetGold() < dwPrice)
#else
	if (ch->GetGold() < dwPrice)
#endif
	{
		sys_log(1, "Shop::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), dwPrice);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}
#ifdef ENABLE_CHEQUE_SYSTEM
	if (ch->GetCheque() < byChequePrice)
	{
		sys_log(1, "Shop::Buy : Not enough cheque : %s has %d, price %d", ch->GetName(), ch->GetCheque(), byChequePrice);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}
#endif
	LPITEM item;

	if (m_pkPC) // 피씨가 운영하는 샵은 피씨가 실제 아이템을 가지고있어야 한다.
		item = r_item.pkItem;
	else
		item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return SHOP_SUBHEADER_GC_SOLD_OUT;



	int iEmptyPos;
	if (item->IsDragonSoul())
	{
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
	}
#ifdef __SPECIAL_STORAGE_SYSTEM__
	else if (item->IsSpecialStorageItem())
		iEmptyPos = ch->GetEmptySpecialStorageSlot(item);
#endif
	else
	{
		iEmptyPos = ch->GetEmptyInventory(item->GetSize());
	}

	if (iEmptyPos < 0)
	{
		if (m_pkPC)
		{
			sys_log(1, "Shop::Buy at PC Shop : Inventory full : %s size %d", ch->GetName(), item->GetSize());
			return SHOP_SUBHEADER_GC_INVENTORY_FULL;
		}
		else
		{
			sys_log(1, "Shop::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
			M2_DESTROY_ITEM(item);
			return SHOP_SUBHEADER_GC_INVENTORY_FULL;
		}
	}
#ifdef ENABLE_CHEQUE_SYSTEM
	if (dwPrice)
		ch->PointChange(POINT_GOLD, -dwPrice, false);
	if (byChequePrice)
		ch->PointChange(POINT_CHEQUE, -byChequePrice, false);
#ifdef ENABLE_MULTISHOP
	if (dwWItemVnum > 0)
		ch->RemoveSpecifyItem(dwWItemVnum, dwWItemPrice);
#endif
#else
	ch->PointChange(POINT_GOLD, -dwPrice, false);
#endif

#ifdef __BATTLE_PASS__
	if (!m_pkPC)
	{
		if (!ch->v_counts.empty())
		{
			for (int i=0; i<ch->missions_bp.size(); ++i)
			{
				if (ch->missions_bp[i].type == 3){	ch->DoMission(i, dwPrice);}
			}
		}
	}
#endif	
	//ch->PointChange(POINT_GOLD, -dwPrice, false);
	/*TShopTable test;
	std::map<int, TShopTable *> map_shop;
	TShopTable * shop_table;
	
	shop_table->dwNPCVnum;

	
	ch->SetQuestNPCID('2');
	quest::CQuestManager::instance().OnBuy(ch->GetPlayerID(), item);
	*/

	

	// 군주 시스템 : 세금 징수
	if (m_pkPC)
	{

		m_pkPC->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);

		char buf[512];

		if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
		{
			snprintf(buf, sizeof(buf), "%s FROM: %u TO: %u PRICE: %u", item->GetName(ch->GetLanguage()), ch->GetPlayerID(), m_pkPC->GetPlayerID(), dwPrice);
			LogManager::instance().GoldBarLog(ch->GetPlayerID(), item->GetID(), SHOP_BUY, buf);
			LogManager::instance().GoldBarLog(m_pkPC->GetPlayerID(), item->GetID(), SHOP_SELL, buf);
		}
		
		item->RemoveFromCharacter();
		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSkillBookItem())
			item->AddToCharacter(ch, TItemPos(SKILLBOOK_INVENTORY, iEmptyPos));
		else if (item->IsUpgradeItem())
			item->AddToCharacter(ch, TItemPos(UPPITEM_INVENTORY, iEmptyPos));
		else if (item->IsGhostStoneItem())
			item->AddToCharacter(ch, TItemPos(GHOSTSTONE_INVENTORY, iEmptyPos));
		else if (item->IsGeneralItem())
			item->AddToCharacter(ch, TItemPos(GENERAL_INVENTORY, iEmptyPos));
#endif
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		if (item->IsSpecialStorageItem())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TEST PORCODIO."));
		}
		

		snprintf(buf, sizeof(buf), "%s %u(%s) %u %u %u", item->GetName(ch->GetLanguage()), m_pkPC->GetPlayerID(), m_pkPC->GetName(), dwPrice, byChequePrice, item->GetCount());
		LogManager::instance().ItemLog(ch, item, "SHOP_BUY", buf);
		// BUY EVENT
		

		snprintf(buf, sizeof(buf), "%s %u(%s) %u %u %u", item->GetName(ch->GetLanguage()), ch->GetPlayerID(), ch->GetName(), dwPrice, byChequePrice, item->GetCount());
		LogManager::instance().ItemLog(m_pkPC, item, "SHOP_SELL", buf);
	

		r_item.pkItem = NULL;
		BroadcastUpdateItem(pos);

#ifdef ENABLE_CHEQUE_SYSTEM
		if (dwPrice)
			m_pkPC->PointChange(POINT_GOLD, dwPrice, false);
		if (byChequePrice)
			m_pkPC->PointChange(POINT_CHEQUE, byChequePrice, false);
#else
		m_pkPC->PointChange(POINT_GOLD, dwPrice, false);
#endif
		
	}
	else
	{
		if (item->IsSpecialStorageItem())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The item was moved to your Special Storage. [Press U]"));
		}

		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSkillBookItem())
			item->AddToCharacter(ch, TItemPos(SKILLBOOK_INVENTORY, iEmptyPos));
		else if (item->IsUpgradeItem())
			item->AddToCharacter(ch, TItemPos(UPPITEM_INVENTORY, iEmptyPos));
		else if (item->IsGhostStoneItem())
			item->AddToCharacter(ch, TItemPos(GHOSTSTONE_INVENTORY, iEmptyPos));
		else if (item->IsGeneralItem())
			item->AddToCharacter(ch, TItemPos(GENERAL_INVENTORY, iEmptyPos));
#endif
		else{
			WORD bCount = item->GetCount();
			if ((item->GetFlag() == 4 || item->GetFlag() == 20))
			{

				for (WORD i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = ch->GetInventoryItem(i);

					if (!item2)
						continue;

					if (item2->GetVnum() == item->GetVnum())
					{
						int j;

						for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
							if (item2->GetSocket(j) != item->GetSocket(j))
								break;

						if (j != ITEM_SOCKET_MAX_NUM)
							continue;

						WORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;

						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
						{
							break;
						}
					}
				}

				item->SetCount(bCount);
			}
			if (bCount > 0)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
			}
			else
				M2_DESTROY_ITEM(item);
		}
		ITEM_MANAGER::instance().FlushDelayedSave(item);
		LogManager::instance().ItemLog(ch, item, "BUY", item->GetName(ch->GetLanguage()));

		if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
		{
			LogManager::instance().GoldBarLog(ch->GetPlayerID(), item->GetID(), PERSONAL_SHOP_BUY, "");
		}

		//DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -dwPrice);
	}

	if (item)
		sys_log(0, "SHOP: BUY: name %s %s(x %d):%u price %u", ch->GetName(), item->GetName(ch->GetLanguage()), item->GetCount(), item->GetID(), dwPrice);

    ch->Save();
    return (SHOP_SUBHEADER_GC_OK);
}


bool CShop::AddGuest(LPCHARACTER ch, DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_START;

	TPacketGCShopStart pack2;

	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = owner_vid;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

#ifdef ENABLE_SHOP_BLACKLIST
		//HIVALUE_ITEM_EVENT
		if (quest::CQuestManager::instance().GetEventFlag("hivalue_item_sell") == 0)
		{
			//축복의 구슬 && 만년한철 이벤트
			if (item.vnum == 70024 || item.vnum == 70035)
			{
				continue;
			}
		}
#endif
		//END_HIVALUE_ITEM_EVENT
		if (m_pkPC && !item.pkItem)
			continue;

		pack2.items[i].vnum = item.vnum;

		// REMOVED_EMPIRE_PRICE_LIFT
#ifdef ENABLE_NEWSTUFF
		if (bOtherEmpire && !g_bEmpireShopPriceTripleDisable) // no empire price penalty for pc shop
#else
		if (bOtherEmpire) // no empire price penalty for pc shop
#endif
		{
#ifdef ENABLE_CHEQUE_SYSTEM
			pack2.items[i].price.dwPrice = item.price * 1;
#else
			pack2.items[i].price = item.price * 1;
#endif
		}
		else
#ifdef ENABLE_CHEQUE_SYSTEM
			pack2.items[i].price.dwPrice = item.price;

		pack2.items[i].price.byChequePrice = item.cheque_price;
#else
			pack2.items[i].price = item.price;
#endif
		// END_REMOVED_EMPIRE_PRICE_LIFT

		pack2.items[i].count = item.count;
#ifdef ENABLE_MULTISHOP
		pack2.items[i].wPriceVnum = item.wPriceVnum;
		pack2.items[i].wPrice = item.wPrice;
#endif
		
		if (item.pkItem)
		{
			thecore_memcpy(pack2.items[i].alSockets, item.pkItem->GetSockets(), sizeof(pack2.items[i].alSockets));
			thecore_memcpy(pack2.items[i].aAttr, item.pkItem->GetAttributes(), sizeof(pack2.items[i].aAttr));
#ifdef CHANGELOOK_SYSTEM
			pack2.items[i].transmutation = item.pkItem->GetTransmutation();
#endif
		}
	}

	pack.size = sizeof(pack) + sizeof(pack2);
	
	
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCShopStart));
	return true;
}

void CShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetShop() != this)
		return;

	m_map_guest.erase(ch);
	ch->SetShop(NULL);

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	ch->GetDesc()->Packet(&pack, sizeof(pack));
}

void CShop::Broadcast(const void * data, int bytes)
{
	sys_log(1, "Shop::Broadcast %p %d", data, bytes);

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;

		if (ch->GetDesc())
			ch->GetDesc()->Packet(data, bytes);

		++it;
	}
}

void CShop::BroadcastUpdateItem(BYTE pos)
{
	TPacketGCShop pack;
	TPacketGCShopUpdateItem pack2;

	TEMP_BUFFER	buf;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_UPDATE_ITEM;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.pos		= pos;

	if (m_pkPC && !m_itemVector[pos].pkItem)
		pack2.item.vnum = 0;
	else
	{
		pack2.item.vnum	= m_itemVector[pos].vnum;
		if (m_itemVector[pos].pkItem)
		{
			thecore_memcpy(pack2.item.alSockets, m_itemVector[pos].pkItem->GetSockets(), sizeof(pack2.item.alSockets));
			thecore_memcpy(pack2.item.aAttr, m_itemVector[pos].pkItem->GetAttributes(), sizeof(pack2.item.aAttr));
		}
		else
		{
			memset(pack2.item.alSockets, 0, sizeof(pack2.item.alSockets));
			memset(pack2.item.aAttr, 0, sizeof(pack2.item.aAttr));
		}
	}

#ifdef ENABLE_CHEQUE_SYSTEM
	pack2.item.price.dwPrice = m_itemVector[pos].price;
	pack2.item.price.byChequePrice = m_itemVector[pos].cheque_price;
#else
	pack2.item.price = m_itemVector[pos].price;
#endif
	pack2.item.count = m_itemVector[pos].count;

#ifdef ENABLE_MULTISHOP
	pack2.item.wPriceVnum = m_itemVector[pos].wPriceVnum;
	pack2.item.wPrice = m_itemVector[pos].wPrice;
#endif

	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));

	Broadcast(buf.read_peek(), buf.size());
}

int CShop::GetNumberByVnum(DWORD dwVnum)
{
	int itemNumber = 0;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

		if (item.vnum == dwVnum)
		{
			itemNumber += item.count;
		}
	}

	return itemNumber;
}

bool CShop::IsSellingItem(DWORD itemID)
{
	bool isSelling = false;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		if ((unsigned int)(m_itemVector[i].itemid) == itemID)
		{
			isSelling = true;
			break;
		}
	}

	return isSelling;

}
