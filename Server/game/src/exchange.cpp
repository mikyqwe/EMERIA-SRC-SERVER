#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "questmanager.h" // @fixme150
#ifdef ENABLE_MESSENGER_BLOCK
#include "messenger_manager.h"
#endif

void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void * pvData = NULL);

// ��ȯ ��Ŷ
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void * pvData)
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header 		= HEADER_GC_EXCHANGE;
	pack_exchg.sub_header 	= sub_header;
	pack_exchg.is_me		= is_me;
	pack_exchg.arg1		= arg1;
	pack_exchg.arg2		= arg2;
	pack_exchg.arg3		= arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(((LPITEM) pvData)->GetWindow(), ((LPITEM) pvData)->GetCell());
#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM) pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM) pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
#ifdef CHANGELOOK_SYSTEM
		pack_exchg.dwTransmutation = ((LPITEM)pvData)->GetTransmutation();
#endif
	}
	else
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
#endif
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
#ifdef CHANGELOOK_SYSTEM
		pack_exchg.dwTransmutation = 0;
#endif
	}

	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
}

// ��ȯ�� ����
bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().CheckMessengerList(GetName(), victim->GetName(), SYST_BLOCK))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%s blokkk"), victim->GetName());
		return false;
	}
	#endif	
	
	if (this == victim)	// �ڱ� �ڽŰ��� ��ȯ�� ���Ѵ�.
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���¿����� ��ȯ�� �� �� �����ϴ�."));
		return false;
	}

	if (victim->IsNPC())
		return false;

	//PREVENT_TRADE_WINDOW
#ifdef OFFLINE_SHOP
	if (IsOpenSafebox() || GetShopOwner() || GetMyShop() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
#else
	if (IsOpenSafebox() || GetShopOwner() || GetMyShop() || IsCubeOpen())
#endif
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT("�ٸ� �ŷ�â�� ����������� �ŷ��� �Ҽ� �����ϴ�." ) );
		return false;
	}

#ifdef OFFLINE_SHOP
	if (victim->IsOpenSafebox() || victim->GetShopOwner() || victim->GetMyShop() || victim->IsCubeOpen() || victim->GetOfflineShopOwner() || victim->GetMailBox())
#else
	if (victim->IsOpenSafebox() || victim->GetShopOwner() || victim->GetMyShop() || victim->IsCubeOpen())
#endif
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT("������ �ٸ� �ŷ����̶� �ŷ��� �Ҽ� �����ϴ�." ) );
		return false;
	}
	//END_PREVENT_TRADE_WINDOW
	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	// �Ÿ� üũ
	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ��ȯ �ź� �����Դϴ�."));
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	//
	SetExchangeTime();
	victim->SetExchangeTime();

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_lGold = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
	m_bCheque = 0;
#endif
	m_pOwner = pOwner;
	pOwner->SetExchange(this);

#ifdef __NEW_EXCHANGE_WINDOW__
	m_pGrid = M2_NEW CGrid(6, 4);
#else
	m_pGrid = M2_NEW CGrid(4, 3);
#endif
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	// ���� ��ȯ�� �� ����
	if (item_pos.IsEquipPosition())
		return false;

	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(m_pOwner->GetLanguage(),"�������� �ǳ��� �� �����ϴ�."));
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	// �̹� ��ȯâ�� �߰��� �������ΰ�?
	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]		= item;
		m_aItemPos[i]		= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner,
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				true,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		exchange_packet(GetCompany()->GetOwner(),
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				false,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);

		return true;
	}

	// �߰��� ������ ����
	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(),	EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]	    = NULL;
	m_aItemPos[pos]	    = NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

bool CExchange::AddGold(long gold)
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		// ������ �ִ� ���� ����.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if (m_lGold > 0)
		return false;

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

#ifdef ENABLE_CHEQUE_SYSTEM
bool CExchange::AddCheque(int Cheque)
{
	if (Cheque <= 0)
		return false;

	if (GetOwner()->GetCheque() < Cheque)
	{
		// ������ �ִ� ���� ����.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_CHEQUE, 0, 0, NPOS, 0);
		return false;
	}

	if (m_bCheque > 0)
		return false;

	Accept(false);
	GetCompany()->Accept(false);

	m_bCheque = Cheque;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_CHEQUE_ADD, true, m_bCheque, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_CHEQUE_ADD, false, m_bCheque, NPOS, 0);
	return true;
}
#endif

// ���� ����� �ִ���, ��ȯ�Ϸ��� �������� ������ �ִ��� Ȯ�� �Ѵ�.
bool CExchange::Check(int * piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

#ifdef ENABLE_CHEQUE_SYSTEM
	if (GetOwner()->GetCheque() < m_bCheque)
		return false;
#endif

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

bool CExchange::CheckSpace()
{
	static CGrid s_grid1(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_grid2(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 2
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	static CGrid s_grid3(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_grid4(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 4
#endif

#ifdef __SPECIAL_STORAGE_SYSTEM__
	static CGrid s_SkillBook_grid1(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_SkillBook_grid2(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 2
	static CGrid s_SkillBook_grid3(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_SkillBook_grid4(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 4

	static CGrid s_Upgrade_grid1(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_Upgrade_grid2(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 2
	static CGrid s_Upgrade_grid3(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_Upgrade_grid4(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 4

	static CGrid s_GhostStone_grid1(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_GhostStone_grid2(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 2
	static CGrid s_GhostStone_grid3(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_GhostStone_grid4(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 4

	static CGrid s_General_grid1(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_General_grid2(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 2
	static CGrid s_General_grid3(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_General_grid4(SPECIAL_STORAGE_INVENTORY_PAGE_COLUMN, SPECIAL_STORAGE_INVENTORY_PAGE_ROW); // inven page 4
#endif

	s_grid1.Clear();
	s_grid2.Clear();
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	s_grid3.Clear();
	s_grid4.Clear();
#endif

#ifdef __SPECIAL_STORAGE_SYSTEM__
	s_SkillBook_grid1.Clear();
	s_SkillBook_grid2.Clear();
	s_SkillBook_grid3.Clear();
	s_SkillBook_grid4.Clear();

	s_Upgrade_grid1.Clear();
	s_Upgrade_grid2.Clear();
	s_Upgrade_grid3.Clear();
	s_Upgrade_grid4.Clear();

	s_GhostStone_grid1.Clear();
	s_GhostStone_grid2.Clear();
	s_GhostStone_grid3.Clear();
	s_GhostStone_grid4.Clear();

	s_General_grid1.Clear();
	s_General_grid2.Clear();
	s_General_grid3.Clear();
	s_General_grid4.Clear();
#endif

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;

	for (i = 0; i < INVENTORY_PAGE_SIZE*1; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid1.Put(i, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*1; i < INVENTORY_PAGE_SIZE*2; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid2.Put(i - INVENTORY_PAGE_SIZE*1, 1, item->GetSize());
	}
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	for (i = INVENTORY_PAGE_SIZE*2; i < INVENTORY_PAGE_SIZE*3; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid3.Put(i - INVENTORY_PAGE_SIZE*2, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*3; i < INVENTORY_PAGE_SIZE*4; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid4.Put(i - INVENTORY_PAGE_SIZE*3, 1, item->GetSize());
	}
#endif
#ifdef __SPECIAL_STORAGE_SYSTEM__
	// SKILLBOOK_INVENTORY
	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, SKILLBOOK_INVENTORY)))
			continue;

		s_SkillBook_grid1.Put(i, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, SKILLBOOK_INVENTORY)))
			continue;

		s_SkillBook_grid2.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, SKILLBOOK_INVENTORY)))
			continue;

		s_SkillBook_grid3.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, SKILLBOOK_INVENTORY)))
			continue;

		s_SkillBook_grid4.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}

	// UPPITEM_INVENTORY
	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, UPPITEM_INVENTORY)))
			continue;

		s_Upgrade_grid1.Put(i, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, UPPITEM_INVENTORY)))
			continue;

		s_Upgrade_grid2.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, UPPITEM_INVENTORY)))
			continue;

		s_Upgrade_grid3.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, UPPITEM_INVENTORY)))
			continue;

		s_Upgrade_grid4.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}

	// GHOSTSTONE_INVENTORY
	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GHOSTSTONE_INVENTORY)))
			continue;

		s_GhostStone_grid1.Put(i, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GHOSTSTONE_INVENTORY)))
			continue;

		s_GhostStone_grid2.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GHOSTSTONE_INVENTORY)))
			continue;

		s_GhostStone_grid3.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GHOSTSTONE_INVENTORY)))
			continue;

		s_GhostStone_grid4.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}

	// GENERAL_INVENTORY
	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GENERAL_INVENTORY)))
			continue;

		s_General_grid1.Put(i, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GENERAL_INVENTORY)))
			continue;

		s_General_grid2.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 1, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GENERAL_INVENTORY)))
			continue;

		s_General_grid3.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}

	for (i = SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetSpecialStorageItem(i, GENERAL_INVENTORY)))
			continue;

		s_General_grid4.Put(i - SPECIAL_STORAGE_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}
#endif
	// ��... ���� ������ ������... ��ȥ�� �κ��� ��� �κ� ���� ���� ���� �� �߸��̴� �Ф�
	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	// �ϴ� ��ȥ���� ��ȯ���� ���� ���ɼ��� ũ�Ƿ�, ��ȥ�� �κ� ����� ��ȥ���� ���� �� �ϵ��� �Ѵ�.
	bool bDSInitialized = false;

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;
		
		#ifdef NEW_ADD_INVENTORY
		int envanterblack;
		if (item->IsDragonSoul())
			envanterblack = victim->GetEmptyDragonSoulInventory(item);
		else
			envanterblack = victim->GetEmptyInventory(item->GetSize());

		if (envanterblack < 0)
			return false;
		#endif

		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

#ifdef ENABLE_EXTENDED_DS_INVENTORY
			for (int i = 0; i < (DRAGON_SOUL_BOX_SIZE * DRAGON_SOUL_INVENTORY_PAGE_COUNT); i++)
#else
			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
#endif
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos])
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] =  wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;
		}
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSkillBookItem())
		{
			int iPos;

			if ((iPos = s_SkillBook_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_SkillBook_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_SkillBook_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_SkillBook_grid2.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_SkillBook_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_SkillBook_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_SkillBook_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_SkillBook_grid4.Put(iPos, 1, item->GetSize());
			}
			else
				return false;
		}
		else if (item->IsUpgradeItem())
		{
			int iPos;

			if ((iPos = s_Upgrade_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_Upgrade_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_Upgrade_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_Upgrade_grid2.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_Upgrade_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_Upgrade_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_Upgrade_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_Upgrade_grid4.Put(iPos, 1, item->GetSize());
			}
			else
				return false;
		}
		else if (item->IsGhostStoneItem())
		{
			int iPos;

			if ((iPos = s_GhostStone_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_GhostStone_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_GhostStone_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_GhostStone_grid2.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_GhostStone_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_GhostStone_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_GhostStone_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_GhostStone_grid4.Put(iPos, 1, item->GetSize());
			}
			else
				return false;
		}
		else if (item->IsGeneralItem())
		{
			int iPos;

			if ((iPos = s_General_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_General_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_General_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_General_grid2.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_General_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_General_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_General_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_General_grid4.Put(iPos, 1, item->GetSize());
			}
			else
				return false;
		}
#endif
		else
		{
			int iPos;

			if ((iPos = s_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid2.Put(iPos, 1, item->GetSize());
			}
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
			else if ((iPos = s_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid4.Put(iPos, 1, item->GetSize());
			}
#endif
			else
				return false;
		}
	}

	return true;
}

// ��ȯ �� (�����۰� �� ���� ������ �ű��)
bool CExchange::Done()
{
	int		empty_pos, i;
	LPITEM	item;

	LPCHARACTER	victim = GetCompany()->GetOwner();

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
			empty_pos = victim->GetEmptyDragonSoulInventory(item);
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSpecialStorageItem())
			empty_pos = victim->GetEmptySpecialStorageSlot(item);
#endif
		else
			empty_pos = victim->GetEmptyInventory(item->GetSize());

		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s",
					m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

		if (item->GetVnum() == 90008 || item->GetVnum() == 90009) // VCARD
		{
			VCardUse(m_pOwner, victim, item);
			continue;
		}

		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
		item->RemoveFromCharacter();

		if (item->IsDragonSoul())
			item->AddToCharacter(victim, TItemPos(DRAGON_SOUL_INVENTORY, empty_pos));
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSpecialStorageItem())
			item->AddToCharacter(victim, TItemPos(item->GetSpecialWindowType(), empty_pos));
#endif
		else
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));

		ITEM_MANAGER::instance().FlushDelayedSave(item);

		item->SetExchanging(false);
		{
			char exchange_buf[51];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);

			if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
			{
				LogManager::instance().GoldBarLog(victim->GetPlayerID(), item->GetID(), EXCHANGE_TAKE, "");
				LogManager::instance().GoldBarLog(GetOwner()->GetPlayerID(), item->GetID(), EXCHANGE_GIVE, "");
			}
		}

		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);

		if (m_lGold > 1000)
		{
			char exchange_buf[51];
			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", GetOwner()->GetPlayerID(), GetOwner()->GetName());
			LogManager::instance().CharLog(victim, m_lGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", victim->GetPlayerID(), victim->GetName());
			LogManager::instance().CharLog(GetOwner(), m_lGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}

#ifdef ENABLE_CHEQUE_SYSTEM
	if (m_bCheque)
	{
		GetOwner()->PointChange(POINT_CHEQUE, -m_bCheque, true);
		victim->PointChange(POINT_CHEQUE, m_bCheque, true);
	}
#endif

	m_pGrid->Clear();
	return true;
}

// ��ȯ�� ����
bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	// �� �� ���� �����Ƿ� ��ȯ ����
	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		// @fixme150 BEGIN
		if (quest::CQuestManager::instance().GetPCForce(GetOwner()->GetPlayerID())->IsRunning() == true)
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"You cannot trade if you're using quests"));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"You cannot trade if you're using quests"));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"You cannot trade if the other part using quests"));
			goto EXCHANGE_END;
		}
		// @fixme150 END

		// exchange_check ������ ��ȯ�� �����۵��� ���ڸ��� �ֳ� Ȯ���ϰ�,
		// ��ũ�� ����� �ֳ� Ȯ���Ѵ�, �ι�° ���ڷ� ��ȯ�� ������ ����
		// �� �����Ѵ�.
		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"���� �����ϰų� �������� ���ڸ��� �����ϴ�."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"������ ���� �����ϰų� �������� ���ڸ��� �����ϴ�."));
			goto EXCHANGE_END;
		}

		// ���� ���� ������ ������ ������ ����ǰ�� ���� �ڸ��� �ֳ� Ȯ���Ѵ�.
		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"������ ����ǰ�� �� ������ �����ϴ�."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"����ǰ�� �� ������ �����ϴ�."));
			goto EXCHANGE_END;
		}

		// ���浵 ����������..
		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"���� �����ϰų� �������� ���ڸ��� �����ϴ�."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"������ ���� �����ϰų� �������� ���ڸ��� �����ϴ�."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"������ ����ǰ�� �� ������ �����ϴ�."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"����ǰ�� �� ������ �����ϴ�."));
			goto EXCHANGE_END;
		}

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		if (Done())
		{
#ifdef ENABLE_CHEQUE_SYSTEM
			if (m_lGold || m_bCheque)
#else
			if (m_lGold) // ���� ���� ���� ����
#endif
				GetOwner()->Save();

			if (GetCompany()->Done())
			{
#ifdef ENABLE_CHEQUE_SYSTEM
				if (GetCompany()->m_lGold || GetCompany()->m_bCheque)
#else
				if (GetCompany()->m_lGold) // ���� ���� ���� ����
#endif
					victim->Save();

				// INTERNATIONAL_VERSION
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetOwner()->GetLanguage(),"%s �԰��� ��ȯ�� ���� �Ǿ����ϴ�."), victim->GetName());
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(victim->GetLanguage(),"%s �԰��� ��ȯ�� ���� �Ǿ����ϴ�."), GetOwner()->GetName());
				// END_OF_INTERNATIONAL_VERSION
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		// �ƴϸ� accept�� ���� ��Ŷ�� ������.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

// ��ȯ ���
void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}

