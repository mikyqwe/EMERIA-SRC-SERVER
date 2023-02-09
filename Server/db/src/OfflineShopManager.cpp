#include "stdafx.h"
#include "DBManager.h"
#include "Peer.h"
#include "OfflineShopManager.h"

CShopOffline::CShopOffline()
{
}

CShopOffline::~CShopOffline()
{
}

void CShopOffline::Initialize()
{
	auction_item_cache_map.clear();
	LoadShopOfflineItems();
	LoadShops();
}

void CShopOffline::LoadShopOfflineItems()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
		"SELECT id,	owner_id, count, vnum, transmutation, socket0, socket1, socket2, "
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
		" socket3 ,"
#endif
		"attrtype0, attrvalue0, "
		"attrtype1, attrvalue1, "
		"attrtype2, attrvalue2, "
		"attrtype3, attrvalue3, "
		"attrtype4, attrvalue4, "
		"attrtype5, attrvalue5, "
		"attrtype6, attrvalue6  "
		"FROM offline_shop_item");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;

	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TPlayerItem item;

		int cur = 0;

		str_to_number(item.id, row[cur++]);
		str_to_number(item.owner, row[cur++]);
		item.window = AUCTION;
		str_to_number(item.count, row[cur++]);
		str_to_number(item.vnum, row[cur++]);
		str_to_number(item.alSockets[0], row[cur++]);
		str_to_number(item.alSockets[1], row[cur++]);
		str_to_number(item.alSockets[2], row[cur++]);
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
		str_to_number(item.alSockets[3], row[cur++]);
#endif
		for (int j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			str_to_number(item.aAttr[j].bType, row[cur++]);
			str_to_number(item.aAttr[j].sValue, row[cur++]);
		}
		InsertItemCache(&item, true);
	}
	return;
}

void CShopOffline::LoadShops()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
		"select * from offline_shop");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;

	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TAuctionItemInfo auctionItemInfo;

		int cur = 0;

		str_to_number(auctionItemInfo.item_num, row[cur++]);
		str_to_number(auctionItemInfo.offer_price, row[cur++]);
		str_to_number(auctionItemInfo.price, row[cur++]);
		str_to_number(auctionItemInfo.offer_id, row[cur++]);
		thecore_memcpy (auctionItemInfo.shown_name, (char*)row[cur], strlen((char*)row[cur]) +1);
		cur++;
		str_to_number(auctionItemInfo.empire, row[cur++]);
		str_to_number(auctionItemInfo.expired_time, row[cur++]);
		str_to_number(auctionItemInfo.item_id, row[cur++]);
		str_to_number(auctionItemInfo.bidder_id, row[cur++]);

		InsertAuctionItemInfoCache(&auctionItemInfo, true);
	}
	return;
}

void CShopOffline::Boot(CPeer* peer)
{
	peer->EncodeWORD(sizeof(TPlayerItem));
	peer->EncodeWORD(auction_item_cache_map.size());

	itertype(auction_item_cache_map) auction_item_cache_map_it = auction_item_cache_map.begin();

	while (auction_item_cache_map_it != auction_item_cache_map.end())
		peer->Encode((auction_item_cache_map_it++)->second->Get(), sizeof(TPlayerItem));

	Auction.Boot(peer);
	Sale.Boot(peer);
}

bool CShopOffline::InsertItemCache(CItemCache *item_cache, bool bSkipQuery)
{
	CItemCache* c = GetItemCache (item_cache->Get(false)->id);
	if (c != NULL)
	{
		return false;
	}
	auction_item_cache_map.insert(TItemCacheMap::value_type(item_cache->Get(true)->id, item_cache));
	item_cache->OnFlush();
	return true;
}

bool CShopOffline::InsertItemCache(TPlayerItem * pNew, bool bSkipQuery)
{
	CItemCache* c = GetItemCache (pNew->id);
	if (c != NULL)
	{
		return false;
	}

	c = new CItemCache();

	c->Put(pNew, bSkipQuery);

	auction_item_cache_map.insert(TItemCacheMap::value_type(pNew->id, c));
	c->Flush();
	return true;
}

bool CShopOffline::DeleteItemCache(DWORD item_id)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		return false;
	}

	c->Delete();

	return true;
}
