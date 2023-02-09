#include "../../libgame/include/grid.h"
#pragma once

#include "db.h"

enum
{
	OFFLINE_SHOP_MAX_DISTANCE = 1500,
};

class COfflineShop
{
	public:
		typedef struct offline_shop_item
		{
			DWORD		owner_id;
			BYTE		pos;
			BYTE		count;
			// BEGIN_MAX_YANG
			long		price;
			// END_OF_MAX_YANG

#ifdef ENABLE_CHEQUE_SYSTEM
			int		cheque;
#endif
			DWORD		vnum;
			bool		bIsSold;

			char					buyerName[CHARACTER_NAME_MAX_LEN];
			long					alSockets[ITEM_SOCKET_MAX_NUM];
			TPlayerItemAttribute	aAttr[ITEM_ATTRIBUTE_MAX_NUM];

#ifdef CHANGELOOK_SYSTEM
			DWORD		transmutation;
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
			DWORD		dwShopVID;
			char		szOwnerName[CHARACTER_NAME_MAX_LEN + 1];
			BYTE		bItemType;
			BYTE		bItemSubType;
			BYTE		bItemLevel;
			BYTE		bItemRefine;
			DWORD		dwFlag;
			DWORD		dwAntiFlag;
			char		szItemName[ITEM_NAME_MAX_LEN + 1];
#endif
			// new function
			DWORD		dwItemID;
			offline_shop_item()
			{
				owner_id = 0;
				pos = 0;
				count = 0;
				price = 0;
				dwItemID = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
				cheque = 0;
#endif
				vnum = 0;
				bIsSold = false;

				memset(buyerName, 0, sizeof(buyerName));
				memset(alSockets, 0, sizeof(alSockets));
				memset(aAttr, 0, sizeof(aAttr));

#ifdef CHANGELOOK_SYSTEM
				transmutation = 0;
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
				dwShopVID = 0;
				memset(szOwnerName, 0, sizeof(szOwnerName));
				bItemType = 0;
				bItemSubType = 0;
				bItemLevel = 0;
				bItemRefine = 0;
				dwFlag = 0;
				dwAntiFlag = 0;
				memset(szItemName, 0, sizeof(szItemName));
#endif
			}
		} OFFLINE_SHOP_ITEM;

		std::vector<OFFLINE_SHOP_ITEM>		m_offlineShopItemVector;

		COfflineShop();
		virtual ~COfflineShop();

		virtual void		SetOfflineShopNPC(LPCHARACTER npc);
		virtual bool		IsOfflineShopNPC(){ return m_pkOfflineShopNPC ? true : false; }

		// BEGIN_MAX_YANG
#ifdef PRIVATESHOP_SEARCH_SYSTEM
		virtual int	Buy(LPCHARACTER ch, BYTE bPos, bool bSearch = false);
#else
		virtual int	Buy(LPCHARACTER ch, BYTE bPos);
#endif
		// END_OF_MAX_YANG

		virtual bool		AddGuest(LPCHARACTER ch, LPCHARACTER npc);
		virtual bool		SendInfoItems(LPCHARACTER ch, LPCHARACTER npc);
		void				RemoveGuest(LPCHARACTER ch);
		void				RemoveAllGuest();
		void				RemoveAllGuestException();

		void				BroadcastUpdateItem(BYTE bPos);
		void				SetShopItems(TShopItemTable * pTable, BYTE bItemCount, LPCHARACTER ch);

		// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
		void				PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice, int bCheque);
#else
		void				PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice);
#endif

#ifdef ENABLE_CHEQUE_SYSTEM
		void				PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice, int bCheque);
#else
		void				PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice);
#endif

#ifdef ENABLE_CHEQUE_SYSTEM
		void				EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice, int bCheque);
#else
		void				EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice);
#endif

		void				DelSingleItem(LPCHARACTER ch, int item_pos);
		void				PutItemsBoot(DWORD dwAccount, DWORD dwOwner);

		// END_OF_MAX_YANG

		void				Destroy(LPCHARACTER npc);
		void				GiveBackMyItems(LPCHARACTER ch);

		bool				CanSaveItems(LPCHARACTER ch);

		std::string shopSign;
		const char *		GetShopSign() { return shopSign.c_str(); };
		void				SetShopSign(const char * c) { shopSign = c; };
	
		// bool				SetClosed(bool s) { bIsClosing = s; };
		
		// bool				GetClosedAuto() { return bAutoExpireStart; };
		// bool				SetClosedAuto(bool s) { bAutoExpireStart = s; };

		// bool				GetIsManage() { return bIsManaging; };
		// bool				SetOwnerManage(bool s) { bIsManaging = s; };

#ifdef PRIVATESHOP_SEARCH_SYSTEM
		std::vector<OFFLINE_SHOP_ITEM> GetItemVector() { return m_offlineShopItemVector; }
#endif

	protected:
		void				Broadcast(const void * data, int bytes);

	private:
		// Grid
		CGrid *				m_pGrid;

		// Guest Map
		typedef TR1_NS::unordered_map<DWORD, bool> GuestMapType;
		GuestMapType m_map_guest;
		// End Of Guest Map

		LPCHARACTER m_pkOfflineShopNPC;
		DWORD	m_dwDisplayedCount;
		// NEW_FUNCTION_SHOP_OFFLINE
		bool	bIsClosing;
		bool	bIsManaging;
		bool	bAutoExpireStart;
		DWORD	dwItems;
		
};
