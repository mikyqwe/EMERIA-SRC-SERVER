#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;
			long long	price;
			WORD	count;
			
			BYTE		money_type;
			DWORD		item_vnum[8];
			DWORD		item_price[8];					

			LPITEM	pkItem;
			int		itemid;	

			shop_item()
			{
				vnum = 0;
				price = 0;
				count = 0;
				itemid = 0;
				money_type = 0;
				memset(&item_vnum, 0, sizeof(item_vnum));
				memset(&item_price, 0, sizeof(item_price));				
				pkItem = NULL;
			}
		} SHOP_ITEM;

		CShop();
		virtual ~CShop(); // @fixme139 (+virtual)

		bool	Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pItemTable);
		void	SetShopItems(TShopItemTable * pItemTable, BYTE bItemCount);

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pkPC ? true : false; }


		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void	RemoveGuest(LPCHARACTER ch);


#ifdef ENABLE_LONG_LONG
		virtual long long	Buy(LPCHARACTER ch, BYTE pos
#ifdef ENABLE_BUY_STACK_FROM_SHOP
, bool multiple = false
#endif
);
#else
		virtual int	Buy(LPCHARACTER ch, BYTE pos
#ifdef ENABLE_BUY_STACK_FROM_SHOP
, bool multiple = false
#endif
);
#endif


		void	BroadcastUpdateItem(BYTE pos);


		int		GetNumberByVnum(DWORD dwVnum);


		virtual bool	IsSellingItem(DWORD itemID);

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

	protected:
		void	Broadcast(const void * data, int bytes);

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;

		CGrid *				m_pGrid;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;

		LPCHARACTER			m_pkPC;
};

#endif
//martysama0134's 2022
