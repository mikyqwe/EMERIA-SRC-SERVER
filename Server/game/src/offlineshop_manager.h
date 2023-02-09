#pragma once
#include "db.h"

class COfflineShopManager : public singleton<COfflineShopManager>
{
	public:
		typedef std::map<DWORD, COfflineShop *> TShopMap;
		typedef std::map<DWORD, DWORD> TOfflineShopMap;

	public:
		COfflineShopManager();
		~COfflineShopManager();

		void			ResetOfflineShopStatus(LPCHARACTER ch);

		LPOFFLINESHOP	CreateOfflineShop(LPCHARACTER npc, DWORD dwOwnerPID);
		LPOFFLINESHOP	FindOfflineShop(DWORD dwVID);
		DWORD			FindMyOfflineShop(DWORD dwPID);

		bool			MapCheck(DWORD mapIndex, DWORD empire);
		bool			ChannelCheck(DWORD dwChannel);
		bool			SearchOfflineShop(LPCHARACTER ch);

		bool			HaveOfflineShopOnAccount(DWORD aID);
		void			InsertOfflineShopToAccount(DWORD aID);
		void			DeleteOfflineShopOnAccount(DWORD aID);
		
		void			BootShop(DWORD idAcc, DWORD dwOwner);

		bool			StartShopping(LPCHARACTER pkChr, LPCHARACTER pkChrShopKeeper);
		void			StopShopping(LPCHARACTER ch);

		void			Buy(LPCHARACTER ch, BYTE bPos);
		// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
		void			PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice, int bCheque);
#else
		void			PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice);
#endif

#ifdef ENABLE_CHEQUE_SYSTEM
		void			EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice, int bCheque);
#else
		void			EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice);
#endif
		void			DelSingleItem(LPCHARACTER ch, int item_pos);
		void			SendItemsEditMode(LPCHARACTER ch);

		void			AutoCloseOfflineShop(LPCHARACTER ch);
		void			AutoCloseOfflineShopBoot(DWORD idAcc, DWORD dwPlayerID);

		// END_OF_MAX_YANG

#ifdef ENABLE_CHEQUE_SYSTEM
		void			PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice, int bCheque);
#else
		void			PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice);
#endif

		void			DestroyOfflineShop(LPCHARACTER ch, DWORD dwVID, DWORD dwPlayerID, bool pcMode = false);

		bool			WithdrawAllMoney(LPCHARACTER ch);
#ifdef ENABLE_CHEQUE_SYSTEM
		bool			WithdrawAllCheque(LPCHARACTER ch);
#endif
		void			FetchMyItems(LPCHARACTER ch);

		void			RefreshMoney(LPCHARACTER ch);

	private:
		TOfflineShopMap	m_Map_pkOfflineShopByNPC2;
		TShopMap		m_map_pkOfflineShopByNPC;

};
