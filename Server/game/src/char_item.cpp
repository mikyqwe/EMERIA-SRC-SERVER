#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#ifdef NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "BattleArena.h"
#include "arena.h"
#include "dev_log.h"
#include "pcbang.h"
#include "threeway_war.h"
#include "safebox.h"
#include "shop.h"
#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#endif
#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/CommonDefines.h"
#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
#endif
#ifdef ENABLE_DECORUM
#include "decorum_arena.h"
#include "decorum.h"
#endif

//auction_temp
#ifdef __AUCTION__
#include "auction_manager.h"
#endif
const int ITEM_BROKEN_METIN_VNUM = 28960;
#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#define FIX_STACKABLE_ITEM
// CHANGE_ITEM_ATTRIBUTES
// const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
// const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};


//��ȯ��, ��ȯ����, ��ȥ����
static bool IS_SUMMON_ITEM(int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	// ��Ű����
	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	// ��
	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 : // ���Ÿ��
		case 71 : // �Ź� ���� 2��
		case 72 : // õ�� ����
		case 73 : // õ�� ���� 2��
		case 193 : // �Ź� ���� 2-1��
#if 0
		case 184 : // õ�� ����(�ż�)
		case 185 : // õ�� ���� 2��(�ż�)
		case 186 : // õ�� ����(õ��)
		case 187 : // õ�� ���� 2��(õ��)
		case 188 : // õ�� ����(����)
		case 189 : // õ�� ���� 2��(����)
#endif
//		case 206 : // �Ʊ͵���
		case 216 : // �Ʊ͵���
		case 217 : // �Ź� ���� 3��
		case 208 : // õ�� ���� (���)

		case 113 : // OX Event ��
			return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	// ��� private ������ ���� �Ұ���
	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	if (!g_bEnableBootaryCheck) return true;

	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}

	return false;
}

// item socket �� ������Ÿ�԰� ������ üũ -- by mhh
static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

// item socket ���� -- by mhh
static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
	// ���� ����
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}
	// ���ڱ���
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || DragonSoul_RefineWindow_GetOpener()!=NULL)
		return false;

#if defined(__BL_MAILBOX__)
	if (GetMailBox())
		return false;
#endif

	if (IsWarping())
		return false;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((m_bAcceCombination) || (m_bAcceAbsorption))
		return false;
#endif
	return true;

#ifdef CHANGELOOK_SYSTEM
	if (m_bChangeLook)
		return false;
#endif
}

#ifdef FAST_EQUIP_WORLDARD 
LPITEM CHARACTER::GetChangeEquipItem(WORD wCell) const
{
	return GetItem(TItemPos(CHANGE_EQUIP,wCell));
}
#endif

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

#ifdef __SPECIAL_STORAGE_SYSTEM__
LPITEM CHARACTER::GetSpecialStorageItem(WORD wCell, int invType) const
{
	return GetItem(TItemPos(invType, wCell));
}
#endif

#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
LPITEM CHARACTER::GetBonus67NewItem() const
{
	return m_pointsInstant.pB67Item;
}
#endif

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return NULL;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pDSItems[wCell];
#ifdef FAST_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		if(wCell >= CHANGE_EQUIP_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid change_equip item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pChangeEquipItem[wCell];
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSwitchbotItems[wCell];
#endif
#ifdef __SPECIAL_STORAGE_SYSTEM__
	case SKILLBOOK_INVENTORY:
		if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SKILLBOOK_INVENTORY item cell %d", wCell);
			return NULL;
		}

		return m_pointsInstant.pSkillBookItems[wCell];

	case UPPITEM_INVENTORY:
		if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid UPPITEM_INVENTORY item cell %d", wCell);
			return NULL;
		}

		return m_pointsInstant.pUppItems[wCell];

	case GHOSTSTONE_INVENTORY:
		if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid GHOSTSTONE_INVENTORY item cell %d", wCell);
			return NULL;
		}

		return m_pointsInstant.pGhostStoneItems[wCell];

	case GENERAL_INVENTORY:
		if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid GENERAL_INVENTORY item cell %d", wCell);
			return NULL;
		}

		return m_pointsInstant.pGeneralItems[wCell];
#endif

	default:
		return NULL;
	}
	return NULL;
}

#ifdef __HIGHLIGHT_SYSTEM__
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem, bool isHighLight)
#else
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem)
#endif
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}
	// �⺻ �κ��丮
	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
							continue;

						m_pointsInstant.bItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 �� �ϴ� ���� ����� üũ�� �� ����
						// �������� ����ó���ϱ� ����
						m_pointsInstant.bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pItems[wCell] = pItem;
		}
		break;
	// ��ȥ�� �κ��丮
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
	case BONUS_NEW_67:
	{
		if (wCell >= BONUS_67_SLOT_MAX)
		{
			sys_err("CHARACTER::SetItem: invalid BONUS_NEW_67 item cell %d", wCell);
			return;
		}

		m_pointsInstant.pB67Item = pItem;
	}
	break;
#endif

#ifdef FAST_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		{
			if (wCell >= CHANGE_EQUIP_SLOT_COUNT)
			{
				sys_err("CHARACTER::SetItem: invalid CHANGE_EQUIP item cell %d", wCell);
				return;
			}
			LPITEM pOld = m_pointsInstant.pChangeEquipItem[wCell];

			if (pOld && pItem)
			{
				return;
			}

			m_pointsInstant.pChangeEquipItem[wCell] = pItem;

		}
		break;	
#endif

	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pDSItems[wCell];

			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
							continue;

						m_pointsInstant.wDSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 �� �ϴ� ���� ����� üũ�� �� ����
						// �������� ����ó���ϱ� ����
						m_pointsInstant.wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pDSItems[wCell] = pItem;
		}
		break;
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		LPITEM pOld = m_pointsInstant.pSwitchbotItems[wCell];
		if (pItem && pOld)
		{
			return;
		}

		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
			return;
		}

		if (pItem)
		{
			CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
		}
		else
		{
			CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
		}

		m_pointsInstant.pSwitchbotItems[wCell] = pItem;
	}
	break;
#endif		
#ifdef __SPECIAL_STORAGE_SYSTEM__
		case SKILLBOOK_INVENTORY:
		{
			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SKILLBOOK_INVENTORY item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pSkillBookItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pSkillBookItems[p] && m_pointsInstant.pSkillBookItems[p] != pOld)
							continue;

						m_pointsInstant.bSkillBookGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bSkillBookGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.bSkillBookGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bSkillBookGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pSkillBookItems[wCell] = pItem;
		}
		break;

		case UPPITEM_INVENTORY:
		{
			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SKILLBOOK_INVENTORY item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pUppItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pUppItems[p] && m_pointsInstant.pUppItems[p] != pOld)
							continue;

						m_pointsInstant.bUppItemsGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bUppItemsGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.bUppItemsGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bUppItemsGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pUppItems[wCell] = pItem;
		}
		break;

		case GHOSTSTONE_INVENTORY:
		{
			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SKILLBOOK_INVENTORY item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pGhostStoneItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pGhostStoneItems[p] && m_pointsInstant.pGhostStoneItems[p] != pOld)
							continue;

						m_pointsInstant.bGhostStoneGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bGhostStoneGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.bGhostStoneGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bGhostStoneGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pGhostStoneItems[wCell] = pItem;
		}
		break;

		case GENERAL_INVENTORY:
		{
			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SKILLBOOK_INVENTORY item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pGeneralItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pGeneralItems[p] && m_pointsInstant.pGeneralItems[p] != pOld)
							continue;

						m_pointsInstant.bGeneraGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bGeneraGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.bGeneraGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bGeneraGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pGeneralItems[wCell] = pItem;
		}
		break;
#endif

	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		// Ȯ�� ������: �������� ������ �÷��� ������ ������
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
#ifdef CHANGELOOK_SYSTEM
			pack.transmutation = pItem->GetTransmutation();
#endif
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags	= pItem->GetAntiFlag();
#ifdef __HIGHLIGHT_SYSTEM__
			if (isHighLight)
				pack.highlight = true;
			else
				pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#endif

			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
#ifdef CHANGELOOK_SYSTEM
			pack.transmutation = 0;
#endif
			pack.vnum = 0;
			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
#ifdef FAST_EQUIP_WORLDARD
		case CHANGE_EQUIP:
			pItem->SetWindow(CHANGE_EQUIP);
			break;
#endif
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
		case BONUS_NEW_67:
			pItem->SetWindow(BONUS_NEW_67);
			break;
#endif
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
#endif		
#ifdef __SPECIAL_STORAGE_SYSTEM__
		case SKILLBOOK_INVENTORY:
			pItem->SetWindow(SKILLBOOK_INVENTORY);
			break;

		case UPPITEM_INVENTORY:
			pItem->SetWindow(UPPITEM_INVENTORY);
			break;

		case GHOSTSTONE_INVENTORY:
			pItem->SetWindow(GHOSTSTONE_INVENTORY);
			break;

		case GENERAL_INVENTORY:
			pItem->SetWindow(GENERAL_INVENTORY);
			break;
#endif
		}
	}
}

LPITEM CHARACTER::GetWear(BYTE bCell) const
{
	// > WEAR_MAX_NUM : ��ȥ�� ���Ե�.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(BYTE bCell, LPITEM item)
{
	// > WEAR_MAX_NUM : ��ȥ�� ���Ե�.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

#ifdef __HIGHLIGHT_SYSTEM__
	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item, false);
#else
	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);
#endif

/* 	if (!item && bCell == WEAR_WEAPON)
	{
		// �Ͱ� ��� �� ���� ���̶�� ȿ���� ���־� �Ѵ�.
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	} */
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	
#ifdef FAST_EQUIP_WORLDARD
	for (i = 0; i < CHANGE_EQUIP_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(CHANGE_EQUIP, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

#endif	
	
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
	item = GetBonus67NewItem();
	if (item)
	{
#ifdef __ENABLE_ITEM_GARBAGE__
		if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
			//continue;
		}
		else
#endif
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
		
	}
#endif

#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(SKILLBOOK_INVENTORY, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);

			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();

			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPPITEM_INVENTORY, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);

			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();

			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(GHOSTSTONE_INVENTORY, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);

			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();

			M2_DESTROY_ITEM(item);
		}
	}

	for (i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(GENERAL_INVENTORY, i))))
		{
#ifdef __ENABLE_ITEM_GARBAGE__
			if (!Garbage<CItem, LPEVENT>::Ref().VerifyObject(item, &item->m_pkDestroyEvent, __FUNCTION__, __LINE__)) {
				continue;
			}
#endif
			item->SetSkipSave(true);

			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();

			M2_DESTROY_ITEM(item);
		}
	}
#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	#ifdef NEW_ADD_INVENTORY
	int malibuclub2 = (90 + (5*Black_Envanter()));
	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			BYTE bCell = Cell.cell;

			// bItemCellA�� 0AI falseAOA�� ����A������a A��C�� + 1 C������ A������CN��U.
			// ��u��o���� iExceptionCell���� 1A�� ��oC�� ��n����CN��U.
			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (NULL == beltItem)
					return false;

				if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_pointsInstant.bItemGrid[bCell])
				{
					if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
			//black
			else if (bCell >= malibuclub2)
				return false;

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / 4);
					do
					{
						BYTE p = bCell + (5 * j);

						if (p >= malibuclub2)
							return false;

						if (p / (INVENTORY_MAX_NUM / 4) != bPage)
							return false;

						if (m_pointsInstant.bItemGrid[p])
							if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// A�ϡ�a�Ƣ� 1AI��e CNA��A�� A��AoCI��A ��IAI��C��I ������E ����AI
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / 4);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= malibuclub2)
						return false;
					if (p / (INVENTORY_MAX_NUM / 4) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	#else
		switch (Cell.window_type)
		{
		case INVENTORY:
			{
				BYTE bCell = Cell.cell;

				// bItemCellA�� 0AI falseAOA�� ����A������a A��C�� + 1 C������ A������CN��U.
				// ��u��o���� iExceptionCell���� 1A�� ��oC�� ��n����CN��U.
				++iExceptionCell;

				if (Cell.IsBeltInventoryPosition())
				{
					LPITEM beltItem = GetWear(WEAR_BELT);

					if (NULL == beltItem)
						return false;

					if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
						return false;

					if (m_pointsInstant.bItemGrid[bCell])
					{
						if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
							return true;

						return false;
					}

					if (bSize == 1)
						return true;

				}
				//black
				else if (bCell >= INVENTORY_MAX_NUM)
					return false;

				if (m_pointsInstant.bItemGrid[bCell])
				{
					if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
					{
						if (bSize == 1)
							return true;

						int j = 1;
						BYTE bPage = bCell / (INVENTORY_MAX_NUM / 4);

						do
						{
							BYTE p = bCell + (5 * j);

							if (p >= INVENTORY_MAX_NUM)
								return false;

							if (p / (INVENTORY_MAX_NUM / 4) != bPage)
								return false;

							if (m_pointsInstant.bItemGrid[p])
								if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
									return false;
						}
						while (++j < bSize);

						return true;
					}
					else
						return false;
				}

				// A�ϡ�a�Ƣ� 1AI��e CNA��A�� A��AoCI��A ��IAI��C��I ������E ����AI
				if (1 == bSize)
					return true;
				else
				{
					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / 4);

					do
					{
						BYTE p = bCell + (5 * j);

						if (p >= INVENTORY_MAX_NUM)
							return false;
						if (p / (INVENTORY_MAX_NUM / 4) != bPage)
							return false;

						if (m_pointsInstant.bItemGrid[p])
							if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
			}
			break;
	#endif
	
#ifdef FAST_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		{
		WORD wCell = Cell.cell;
		if (wCell >= CHANGE_EQUIP_SLOT_COUNT)
		{
			return false;
		}

		if (m_pointsInstant.pChangeEquipItem[wCell])
		{
			return false;
		}

		return true;
		}
#endif	
	
	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			// bItemCell�� 0�� false���� ��Ÿ���� ���� + 1 �ؼ� ó���Ѵ�.
			// ���� iExceptionCell�� 1�� ���� ���Ѵ�.
			iExceptionCell++;

			if (m_pointsInstant.wDSItemGrid[wCell])
			{
				if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.wDSItemGrid[p])
							if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// ũ�Ⱑ 1�̸� ��ĭ�� �����ϴ� ���̹Ƿ� �׳� ����
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
		WORD wCell = Cell.cell;
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			return false;
		}

		if (m_pointsInstant.pSwitchbotItems[wCell])
		{
			return false;
		}

		return true;
		}
#endif		
#ifdef __SPECIAL_STORAGE_SYSTEM__
		break;

		case SKILLBOOK_INVENTORY:
		{
			WORD wCell = Cell.cell;

			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_pointsInstant.bSkillBookGrid[wCell])
			{
				if (m_pointsInstant.bSkillBookGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.bSkillBookGrid[p])
						{
							if (m_pointsInstant.bSkillBookGrid[p] != iExceptionCell)
								return false;
						}
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
					{
						if (m_pointsInstant.bSkillBookGrid[p] != iExceptionCell)
							return false;
					}
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

		case UPPITEM_INVENTORY:
		{
			WORD wCell = Cell.cell;

			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_pointsInstant.bUppItemsGrid[wCell])
			{
				if (m_pointsInstant.bUppItemsGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.bUppItemsGrid[p])
						{
							if (m_pointsInstant.bUppItemsGrid[p] != iExceptionCell)
								return false;	
						}
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
					{
						if (m_pointsInstant.bUppItemsGrid[p] != iExceptionCell)
							return false;
					}
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

		case GHOSTSTONE_INVENTORY:
		{
			WORD wCell = Cell.cell;

			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_pointsInstant.bGhostStoneGrid[wCell])
			{
				if (m_pointsInstant.bGhostStoneGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.bGhostStoneGrid[p])
						{
							if (m_pointsInstant.bGhostStoneGrid[p] != iExceptionCell)
								return false;
						}
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
					{
						if (m_pointsInstant.bGhostStoneGrid[p] != iExceptionCell)
							return false;
					}
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

		case GENERAL_INVENTORY:
		{
			WORD wCell = Cell.cell;

			if (wCell >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
				return false;

			iExceptionCell++;

			if (m_pointsInstant.bGeneraGrid[wCell])
			{
				if (m_pointsInstant.bGeneraGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.bGeneraGrid[p])
						{
							if (m_pointsInstant.bGeneraGrid[p] != iExceptionCell)
								return false;
						}
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_STORAGE_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
					{
						if (m_pointsInstant.bGeneraGrid[p] != iExceptionCell)
							return false;
					}
				}
				while (++j < bSize);

				return true;
			}
		}
#endif
	}
	return false;
}

#ifdef NEW_ADD_INVENTORY
int CHARACTER::GetEmptyInventory(BYTE size) const
{
	int malibuclub = 90 + (5*Black_Envanter());
	for ( int i = 0; i < malibuclub; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}
#else
int CHARACTER::GetEmptyInventory(BYTE size) const
{
	// NOTE: CoAc AI CO��o��A ����AIAU Ao����, E���쩡 ��iAC CaA�ע�| CO �ҡ� AI����Aa����AC ��o A��A�� A�̡�a A��C�� ��c��e��C��i AO��A���,
	//		���ר��� AI����Aa���碥A ��?��o AI����Aa����AI��C��I ��E��cCIAo ��E��졤I CN��U. (��a���� AI����Aa����: INVENTORY_MAX_NUM ��iAo���� ��E��c)
	for ( int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}
#endif

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;
#ifdef ENABLE_EXTENDED_DS_INVENTORY
	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE * DRAGON_SOUL_INVENTORY_PAGE_COUNT; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;
#else
	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;
#endif
	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

#ifdef __SPECIAL_STORAGE_SYSTEM__
int CHARACTER::GetEmptySpecialStorageSlot(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->GetSpecialWindowType())
		return -1;

	BYTE bSize = pItem->GetSize();
	sys_err("Size check %d", bSize);

	for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
	{
		if (IsEmptyItemGrid(TItemPos(pItem->GetSpecialWindowType(), i), bSize))
			return i;
	}

	return -1;
}
#endif

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;
#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (int i = 0; i < malibuclub; ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)	
#endif
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

#ifdef NEW_ADD_INVENTORY
	return (malibuclub - count);
#else
	return (INVENTORY_MAX_NUM - count);
#endif
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		// ���⼭ �������� �ڵ������� û�� ��
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		// ���� ����
		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	// ���� ������ ����
	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(ch->GetLanguage()), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(ch->GetLanguage()), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	//���� �ð����� : upgrade_refine_scroll.quest ���� ������ 5���̳��� �Ϲ� ������
	//�����Ҽ� ����
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

#ifdef __SPECIALSTAT_SYSTEM__
	int prob_success = prt->prob;
	prob_success = MIN(100, GetSpecialStats(SPECIALSTAT1) + prob_success);
#endif

	// REFINE_COST
	int cost = ComputeRefineFee(prt->cost);

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");

	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���� ��ȸ�� 20 ������ ���⸸ �����մϴ�"));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	int pos = GetEmptyInventory(item->GetSize());
	
	if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
	{
		if (-1 == pos)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("��OAoC�Ƣ��� ��o �Ʃ��ơ�AI ������A��I��U."));
			return false;
		}
	}
	
	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ϱ� ���� ���� �����մϴ�."));
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ϱ� ���� ��ᰡ �����մϴ�."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	// END_OF_REFINE_COST

#ifdef __SPECIALSTAT_SYSTEM__
	if (prob <= prob_success)
#else
	if (prob <= prt->prob)
#endif
	{
		// ����! ��� �������� �������, ���� �Ӽ��� �ٸ� ������ ȹ��
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

#ifdef __BATTLE_PASS__
			if (!v_counts.empty())
			{
				for (int i=0; i<missions_bp.size(); ++i)
				{
					if (missions_bp[i].type == 14){	DoMission(i, 1);}
				}
			}
#endif
			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
			{
				bool bFound = false;
				item->SetCount(item->GetCount() - 1);

				BYTE bCount = pkNewItem->GetCount();

				for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = GetInventoryItem(i);

					if (!item2)
						continue;

					if (item2->GetVnum() == pkNewItem->GetVnum())
					{
						int j;

						for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
							if (item2->GetSocket(j) != pkNewItem->GetSocket(j))
								break;

						if (j != ITEM_SOCKET_MAX_NUM)
							continue;

						BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;

						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
							bFound = true;
					}
				}
				if (!bFound)
					pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, pos));
			}
			else
			{
				ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
				pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			}
			// END_OF_DETAIL_REFINE_LOG

#ifdef CHANGELOOK_SYSTEM
			pkNewItem->SetTransmutation(item->GetTransmutation()); //YANSITMA
#endif
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
		
			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			sys_log(0, "PayPee End %d", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG
			// ������ ������ ���� -> ���� ���з� ����
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		// ����! ��� �������� �����.
		DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		item->AttrLog();
		if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
			item->SetCount(item->GetCount() - 1);
		else
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN   = 1, // �߱������� ���
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL   = 3,
	YAGONG_SCROLL  = 4,
	MEMO_SCROLL	   = 5,
	BDRAGON_SCROLL	= 6,
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	//���� �ð����� : upgrade_refine_scroll.quest ���� ������ 5���̳��� �Ϲ� ������
	//�����Ҽ� ����
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pkItemScroll;

	// ������ üũ
	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	int pos = GetEmptyInventory(item->GetSize());
	if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
	{
		if (-1 == pos)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("��OAoC�Ƣ��� ��o �Ʃ��ơ�AI ������A��I��U."));
			return false;
		}
	}
	
	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� �� �̻� ������ �� �����ϴ�."));
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ���������� ������ �� �����ϴ�."));
			return false;
		}
	}

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
		return false;
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ϱ� ���� ���� �����մϴ�."));
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ϱ� ���� ��ᰡ �����մϴ�."));
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN ||
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL ||
		pkItemScroll->GetValue(0) == YAGONG_SCROLL) // ��ö, ����� �ູ��, �߰��� ������  ó��
	{
		const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };
		const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };

		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			success_prob = hyuniron_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			success_prob = yagong_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) {} // @fixme121
		else
		{
			sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (test_server)
		{
			ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
		}
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) // ��ö�� �������� �μ����� �Ѵ�.
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL) // ������ �ູ���� 100% ���� (+4������)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

#ifdef __SPECIALSTAT_SYSTEM__
	success_prob = MIN(100, GetSpecialStats(SPECIALSTAT1) + success_prob);
#endif

	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

	if (prob <= success_prob)
	{
		// ����! ��� �������� �������, ���� �Ӽ��� �ٸ� ������ ȹ��
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{

			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

#ifdef __BATTLE_PASS__
			if (!v_counts.empty())
			{
				for (int i=0; i<missions_bp.size(); ++i)
				{
					if (missions_bp[i].type == 14){	DoMission(i, 1);}
				}
			}
#endif

			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
			{
				bool bFound = false;
				item->SetCount(item->GetCount() - 1);

				BYTE bCount = pkNewItem->GetCount();

				for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = GetInventoryItem(i);

					if (!item2)
						continue;

					if (item2->GetVnum() == pkNewItem->GetVnum())
					{
						int j;

						for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
							if (item2->GetSocket(j) != pkNewItem->GetSocket(j))
								break;

						if (j != ITEM_SOCKET_MAX_NUM)
							continue;

						BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;

						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
							bFound = true;
					}
				}
				if (!bFound)
					pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, pos));
			}
			else
			{
				ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
				pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			}
#ifdef CHANGELOOK_SYSTEM
			pkNewItem->SetTransmutation(item->GetTransmutation()); //YANSITMA
#endif
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// ������ ������ ���� -> ���� ���з� ����
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		// ����! ��� �������� �������, ���� �Ӽ��� ���� ����� ������ ȹ��
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE FAIL", pkNewItem->GetName());

			BYTE bCell = item->GetCell();
#ifdef CHANGELOOK_SYSTEM
			pkNewItem->SetTransmutation(item->GetTransmutation()); //YANSITMA
#endif

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			NotifyRefineFail(this, item, szRefineType, -1);
			if (item->GetType() == ITEM_METIN && item->GetSubType() == METIN_NORMAL)
			{
				bool bFound = false;
				item->SetCount(item->GetCount() - 1);

				BYTE bCount = pkNewItem->GetCount();

				for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = GetInventoryItem(i);

					if (!item2)
						continue;

					if (item2->GetVnum() == pkNewItem->GetVnum())
					{
						int j;

						for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
							if (item2->GetSocket(j) != pkNewItem->GetSocket(j))
								break;

						if (j != ITEM_SOCKET_MAX_NUM)
							continue;

						BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;

						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
							bFound = true;
					}
				}
				if (!bFound)
					pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, pos));
			}
			else 
			{
				ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");
				pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			}
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			pkNewItem->AttrLog();

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// ������ ������ ���� -> ���� ���з� ����
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType); // ������ ������ ������� ����

		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell)
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	// REFINE_COST
	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� Ÿ�� �Ϸ� ������ �ѹ����� ��밡���մϴ�."));
		return false;
	}
	// END_OF_REFINE_COST

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� �� ������δ� ������ �� �����ϴ�."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �������� ��ĥ ���� �����ϴ�."));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ູ�� ���� ��ö�� ��ĥ �� �ֽ��ϴ�."));
				return false;
			}
		}
	}

	CRefineManager & rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET %d", item->GetRefineSet());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
		return false;
	}

	// REFINE_COST

	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		// �Ϻ��� ����
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���� ��ȸ�� 20 ������ ���⸸ �����մϴ�"));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);

	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

#ifdef __SPECIALSTAT_SYSTEM__
	p.prob = MIN(100, p.prob + GetSpecialStats(SPECIALSTAT1));
#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// XXX ����, ���� �������� ��������ϴ�...
		// XXX ���ɰ������� �ູ�� ���� �Ǿ���!
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell());
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT", pkTarget->GetName());

		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);
					// �������� ��ü���ش�
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �� �ִ� ��ƾ���� �����ϴ�."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "kill_campfire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����� �� �� ���� ��ġ �Դϴ�."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)	// �������� �ϳ���� �׳� ����.
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1) // �׷��� �ʴٸ� �ٸ� �κ��丮 ������ ã�´�.
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����ǰ�� �� ������ �����ϴ�."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
		// �Ƿ決�� �϶�
		case 301:
		case 302:
		case 303:
		case 304:
			if( GetLevel() < 90 )
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� ���� ���Ѻ��� ������ �����ϴ�."));
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��ġ�� Ÿ������ ���� �־ ��ȯ�� �� �����ϴ�."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
#ifdef ENABLE_OPEN_SHOP_WITH_ARMOR
	ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
#else
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ����� ���� ������ �� �� �ֽ��ϴ�."));
			break;
	}
#endif
}

// MYSHOP_PRICE_LIST
#ifdef ENABLE_CHEQUE_SYSTEM
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, TItemPriceType ItemPrice)
#else
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
#endif
{
	char szLine[256];
#ifdef ENABLE_CHEQUE_SYSTEM
	snprintf(szLine, sizeof(szLine), "MyShopPriceListNew %u %u %u", dwItemVnum, ItemPrice.dwPrice, ItemPrice.byChequePrice);
#else
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
#endif
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

//
// DB ĳ�÷� ���� ���� ����Ʈ�� User ���� �����ϰ� ������ ����� Ŀ�ǵ带 ������.
//
void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)
		// ���� ����Ʈ�� ����. dummy �����͸� ���� Ŀ�ǵ带 �����ش�.
#ifdef ENABLE_CHEQUE_SYSTEM
		SendMyShopPriceListCmd(1, TItemPriceType());
#else
		SendMyShopPriceListCmd(1, 0);
#endif
	else {
		for (int idx = 0; idx < p->byCount; idx++)
#ifdef ENABLE_CHEQUE_SYSTEM
			SendMyShopPriceListCmd(pInfo[idx].dwVnum, TItemPriceType(pInfo[idx].price.dwPrice, pInfo[idx].price.byChequePrice));
#else
			SendMyShopPriceListCmd(pInfo[idx].dwVnum, pInfo[idx].dwPrice);
#endif
	}

	__OpenPrivateShop();
}

//
// �̹� ���� �� ó�� ������ Open �ϴ� ��� ����Ʈ�� Load �ϱ� ���� DB ĳ�ÿ� �������� ����Ʈ ��û ��Ŷ�� ������.
// ���ĺ��ʹ� �ٷ� ������ ����� ������ ������.
//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	} else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

#ifdef ENABLE_NEW_AFFECT_POTION
void CHARACTER::SetAffectPotion(LPITEM item)
{
	if (!item)
		return;

	int iPotionAffects[] = {AFFECT_POTION_1, AFFECT_POTION_2, AFFECT_POTION_3, AFFECT_POTION_4, AFFECT_POTION_5, AFFECT_POTION_6};
	int iPotionVnums[] = {50821, 50822, 50823, 50824, 50825, 50826};
	
	for (int i = 0; i < _countof(iPotionVnums); i++)
	{
		if (item->GetVnum() == iPotionVnums[i])
			AddAffect(iPotionAffects[i], APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);		
	}
}
#endif

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"���� ����� ���� ���ڶ� ����� �� �����ϴ�."));
			return -1;
		}

		consumeLife = needLife;


		// CheckMinLifeForWarp: ���� ���ؼ� ������ �ȵǹǷ� ����� �ּҷ��� �����ش�
		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(lpChar->GetLanguage(),"���� ���ŷ� ���� ���ڶ� ����� �� �����ϴ�."));
		return -1;
	}

	return needSP;
}

// #define ENABLE_FIREWORK_STUN
#define ENABLE_ADDSTONE_FAILURE
bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
#ifdef ENABLE_RENEWAL_PVP
	if (CheckPvPUse(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1005"));
		return false;
	}
#endif

	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� ���� ���Ѻ��� ������ �����ϴ�."));
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				//iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %s, Inven %d, Cell %d, ItemType %d, SubType %d", item->GetName(GetLanguage()), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

#ifdef ENABLE_DECORUM
	if ( CDecoredArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
		return false;
	}
#endif

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� �߿��� �̿��� �� ���� ��ǰ�Դϴ�."));
		return false;
	}
#ifdef ENABLE_NEWSTUFF
	else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && IsLimitedPotionOnPVP(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, "Non puoi usare le pozioni durante un duello.");
		return false;
	}
#endif

	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		ChatPacket(CHAT_TYPE_INFO, "Affects are not loaded yet!");
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you have no equipped belt.");
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you don't upgrade your belt.");
			return false;
		}
	}
	// @fixme141 END

	// ������ ���� ��� ���ĺ��ʹ� ������� �ʾƵ� �ð��� �����Ǵ� ��� ó��.
	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		// �� ���̶� ����� ���������� ���δ� Socket1�� ���� �Ǵ��Ѵ�. (Socket1�� ���Ƚ�� ���)
		if (0 == item->GetSocket(1))
		{
			// ��밡�ɽð��� Default ������ Limit Value ���� ����ϵ�, Socket0�� ���� ������ �� ���� ����ϵ��� �Ѵ�. (������ ��)
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);
	}
	
#ifdef __BATTLE_PASS__
	if (!v_counts.empty())
	{
		for (int i=0; i<missions_bp.size(); ++i)
		{
			if (missions_bp[i].type == 1 && item->GetVnum() == missions_bp[i].vnum)
			{
				DoMission(i, 1);
			}
		}
	}
#endif	
	
#ifdef NEW_PET_SYSTEM	
	if (item->GetVnum() == 55001) {

		LPITEM item2;

		if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
			return false;

		if (item2->IsExchanging())
			return false;

		if (item2->GetVnum() > 55710 || item2->GetVnum() < 55701)
			return false;

		
		char szQuery1[1024];
		snprintf(szQuery1, sizeof(szQuery1), "SELECT duration FROM new_petsystem WHERE id = %lu LIMIT 1", item2->GetID());
		std::auto_ptr<SQLMsg> pmsg2(DBManager::instance().DirectQuery(szQuery1));
		if (pmsg2->Get()->uiNumRows > 0) {
			MYSQL_ROW row = mysql_fetch_row(pmsg2->Get()->pSQLResult);
			if (atoi(row[0]) > 0) {
				if (GetNewPetSystem()->IsActivePet()) {
					ChatPacket(CHAT_TYPE_INFO, "You have to unsummon your pet first.");
					return false;
				}

				DBManager::instance().DirectQuery("UPDATE new_petsystem SET duration =(tduration) WHERE id = %d", item2->GetID());
				ChatPacket(CHAT_TYPE_INFO, "Your Pet's life is now full.");
			}
			else {
				DBManager::instance().DirectQuery("UPDATE new_petsystem SET duration =(tduration/2) WHERE id = %d", item2->GetID());
				ChatPacket(CHAT_TYPE_INFO, "Your Pet's life is now restored.");
			}
			item->SetCount(item->GetCount() - 1);
			return true;
		}
		else
			return false;
	}

	if (item->GetVnum() >= 55701 && item->GetVnum() <= 55710) {
		LPITEM item2;

		if ((item2 = GetItem(DestCell))) {
			if (item2->GetVnum() == 55002) {
                if(item->GetAttributeValue(3) < 1){
                    ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"PET_SURESI_BITMISKEN_BUNU_YAPAMASSIN"));
                    return false;
                }
				if(item2->GetAttributeValue(0) > 0){
					ChatPacket(CHAT_TYPE_INFO, "Cutia are deja un animalut inauntru.");
				}
				else{
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging())
						return false;

					if (GetNewPetSystem()->IsActivePet()) {
						ChatPacket(CHAT_TYPE_INFO, "You have to unsummon your pet first.");
						return false;
					}
					
					for (int b = 0; b < 3; b++) {
						item2->SetForceAttribute(b, 1, item->GetAttributeValue(b));
					}
					std::auto_ptr<SQLMsg> pEvo(DBManager::instance().DirectQuery("SELECT evolution FROM new_petsystem WHERE id = %d", item->GetID()));
					MYSQL_ROW evo = mysql_fetch_row(pEvo->Get()->pSQLResult);
					BYTE evolution = 0;
					str_to_number(evolution, evo[0]);
					//char szQuery_evo[1024];
					//snprintf(szQuery_evo, sizeof(szQuery1), "SELECT evolution FROM new_petsystem WHERE id = %d", item->GetID());
					item2->SetForceAttribute(3, 1, item->GetAttributeValue(3));
					item2->SetForceAttribute(4, 1, item->GetAttributeValue(4));
					DWORD vnum1 = item->GetVnum()-55700;
					item2->SetSocket(0, vnum1);
					item2->SetSocket(1, item->GetSocket(1));
					item2->SetSocket(2, evolution);
					//ChatPacket(CHAT_TYPE_INFO, "Pet %d %d %d //// %d %d %d",item->GetAttributeValue(0),item->GetAttributeValue(1),item->GetAttributeValue(2),item2->GetAttributeValue(0),item2->GetAttributeValue(1),item2->GetAttributeValue(2));
					
					// Setting transport box ID as new SummonItem ID (temporary)
					delete(DBManager::instance().DirectQuery("UPDATE new_petsystem SET id =%d WHERE id = %d", item2->GetID(), item->GetID()));
					ITEM_MANAGER::instance().RemoveItem(item);
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
					item2->SetSocket(3, item2->GetID());
#endif					
					return true;
				}
			}
		}
	}
		
		
	

	if (item->GetVnum() == 55002 && item->GetAttributeValue(0) > 0) {
		
		
		
		int pos = GetEmptyInventory(item->GetSize());
		if(pos == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, "You don't have enought space.");
			return false;
		}
		
/*         if(GetLevel() < item->GetAttributeValue(4))
        {
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"pet leveli senin levelinden buyuk"));
            return false;
        }		 */

		if (item->IsExchanging())
			return false;
		
		DWORD vnum2 = 55700+item->GetSocket(0);
		LPITEM item2 = AutoGiveItem(vnum2, 1);
		for (int b = 0; b < 3; b++) {
			item2->SetForceAttribute(b, 1, item->GetAttributeValue(b));
		}
		item2->SetForceAttribute(3, 1, item->GetAttributeValue(3));
		item2->SetForceAttribute(4, 1, item->GetAttributeValue(4));
		item2->SetSocket(1,item->GetSocket(1));
		//ChatPacket(CHAT_TYPE_INFO, "Pet1 %d %d %d",item->GetAttributeValue(0),item->GetAttributeValue(1),item->GetAttributeValue(2));
		//fixing memory leak
#ifdef ENABLE_FIX_PET_TRANSPORT_BOX
		delete(DBManager::instance().DirectQuery("UPDATE new_petsystem SET id =%d WHERE id = %d", item2->GetID(), item->GetSocket(3) != 0 ? item->GetSocket(3) : item->GetID()));
#else
		delete(DBManager::instance().DirectQuery("UPDATE new_petsystem SET id =%d WHERE id = %d", item2->GetID(), item->GetID()));
#endif
		ITEM_MANAGER::instance().RemoveItem(item);
		return true;
	}
#endif	
	
#ifndef UNFROZEN_SASHES
	#define UNFROZEN_SASHES
#endif
#ifdef __FROZENBONUS_SYSTEM__
#ifdef UNFROZEN_SASHES
	if (item->GetVnum() == FROZEN_ITEM_VNUM || item->GetVnum() == UNFROZEN_ITEM_VNUM || item->GetVnum() == UNFROZEN_ITEM_VNUM_2) {
#else
	if (item->GetVnum() == FROZEN_ITEM_VNUM || item->GetVnum() == UNFROZEN_ITEM_VNUM) {
#endif
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ŷ�â,â�� ���� �� ���¿����� ��ȯ��,��ȯ���� �� ����Ҽ� �����ϴ�."));
			return false;
		}

		LPITEM item2;

		item2 = GetItem(DestCell);

		if (item2){

			sys_log(0, "frozenmanagging");
			if (!IsValidItemPosition(DestCell))
				return false;

			if (item2->IsExchanging())
				return false;

#ifdef UNFROZEN_SASHES
			if (!((item2->GetType() == ITEM_WEAPON && !(item2->GetSubType() == WEAPON_ARROW)) || item2->GetType() == ITEM_ARMOR || item2->GetType() == ITEM_COSTUME))
#else
			if (!((item2->GetType() == ITEM_WEAPON && !(item2->GetSubType() == WEAPON_ARROW)) || item2->GetType() == ITEM_ARMOR))
#endif
				return false;

			switch (item->GetVnum()) {
			case FROZEN_ITEM_VNUM:
#ifdef UNFROZEN_SASHES
				if (item2->GetType() == ITEM_COSTUME){
					return false;
				}
#endif
				if (item2->GetFrozenAttribute() >= 2) {
					ChatPacket(CHAT_TYPE_INFO, "Puoi congelare al massimo 2 bonus.");
					return false;
				}
				if (item2->IsFrozenBonus(item->GetSocket(0))) {
					ChatPacket(CHAT_TYPE_INFO, "Questo Item possiede gia' questo slot congelato.");
					return false;
				}
				if (!item2->SetFrozenBonus(item->GetSocket(0), true)) {
					ChatPacket(CHAT_TYPE_INFO, "Gli slot 1 e 2 non posso essere congelati su questo oggetto.");
					return false;
				}
				item->SetCount(item->GetCount() - 1);
				break;
			case UNFROZEN_ITEM_VNUM:
#ifdef UNFROZEN_SASHES
				if (item2->GetType() == ITEM_COSTUME){
					return false;
				}
#endif
				if (!item2->IsFrozenBonus(item->GetSocket(0))) {
					ChatPacket(CHAT_TYPE_INFO, "Questo Item non possiede questo slot congelato.");
					return false;
				}
				if (!item2->SetFrozenBonus(item->GetSocket(0), false)) {
					ChatPacket(CHAT_TYPE_INFO, "Gli slot 1 e 2 non posso essere scongelati su questo oggetto.");
					return false;
				}
				item->SetCount(item->GetCount() - 1);
				break;
#ifdef UNFROZEN_SASHES
			case UNFROZEN_ITEM_VNUM_2:
				if (item2->GetType() == ITEM_COSTUME && item2->GetSubType() == COSTUME_ACCE){
					if (!item2->IsFrozenBonus(item->GetSocket(0))) {
						ChatPacket(CHAT_TYPE_INFO, "Questo Item non possiede questo slot congelato.");
						return false;
					}
					if (!item2->SetFrozenBonus(item->GetSocket(0), false)) {
						ChatPacket(CHAT_TYPE_INFO, "Gli slot 1 e 2 non posso essere scongelati su questo oggetto.");
						return false;
					}
					item->SetCount(item->GetCount() - 1);
				}
				else{
					ChatPacket(CHAT_TYPE_INFO, "Puoi usarlo solo sulle stole.");
					return false;
				}
				break;
#endif
			}
			return true;
		}
	}
#endif

#ifdef __SPECIALSTAT_SYSTEM__

	if ((item->GetVnum() >= SPECIALSTAT_SKILLBOOK_START && item->GetVnum() < SPECIALSTAT_SKILLBOOK_START + SPECIALSTATS_MAX) || item->GetVnum() == SPECIALSTAT_SKILLBOOK_TIME_SKIP) {
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ŷ�â,â�� ���� �� ���¿����� ��ȯ��,��ȯ���� �� ����Ҽ� �����ϴ�."));
			return false;
		}
		switch (item->GetVnum()) {
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT1:
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT2:
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT3:
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT4:
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT5:
		case SPECIALSTAT_SKILLBOOK_START + SPECIALSTAT6:
			if (!CanReadSpecialStatBooks()) {
				ChatPacket(CHAT_TYPE_INFO, "Non puoi leggere ancora questo libro.");
				return false;
			}
			if (!CanUpdateSpecialStat(item->GetVnum() - SPECIALSTAT_SKILLBOOK_START)) {
				ChatPacket(CHAT_TYPE_INFO, "Questa abilita' ha gia' raggiunto il livello massimo.");
				return false;
			}
			item->SetCount(item->GetCount() - 1);
			SetSpecialStatsNextTimeReadBook();
			if (number(1, 100) <= SPECIALSTAT_SKILLBOOK_PROB) {
				ChatPacket(CHAT_TYPE_INFO, "Hai letto questo libro con successo.");
				UpdateSpecialStat(item->GetVnum() - SPECIALSTAT_SKILLBOOK_START);
			}
			else {
				ChatPacket(CHAT_TYPE_INFO, "Non sei riuscito a leggere questo libro.");
			}
			return true;
			break;
		case SPECIALSTAT_SKILLBOOK_TIME_SKIP:
			if (CanReadSpecialStatBooks()) {
				ChatPacket(CHAT_TYPE_INFO, "Puoi gia' leggere un libro.");
				return false;
			}
			item->SetCount(item->GetCount() - 1);
			if (number(1, 100) <= SPECIALSTAT_SKILLBOOK_TIME_SKIP_PROB) {
				SetSpecialStatsSkillBookTimeSkip();
				ChatPacket(CHAT_TYPE_INFO, "Puoi leggere un altro libro dei talenti.");
			}
			else {
				ChatPacket(CHAT_TYPE_INFO, "Questo libro sembra marcio.");
			}
			return true;
			break;

		}
	}
#endif // __SPECIALSTAT_SYSTEM__

	switch (item->GetType())
	{
		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
#ifdef DISABLE_HORSE_DECORUM
			if (GetDecorumArena() != NULL || IsObserverMode() == true || GetArena() != NULL)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
					return false;
				}
			}
#else
			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello."));
					return false;
				}
			}

#endif
#if defined(__BL_MAILBOX__)
			if (item->GetVnum() == MOBILE_MAILBOX)
			{
				CMailBox::Open(this);
			}
#endif
			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				sys_log(0, "callingitemusequest");
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
			break;

		case ITEM_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ں��� �ǿ� �� ���� �����Դϴ�."));
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �ӿ� ��ں��� �ǿ� �� �����ϴ�."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_RESIST_MAGIC:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049: // ��ܺ�����
										if (g_bEnableBootaryCheck)
										{
											if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
											{
												UseSilkBotary();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ �� �� ���� �����Դϴ�"));
											}
										}
										else
										{
											UseSilkBotary();
										}
										break;
								}
							}
							else
							{
								if (!item->IsEquipped())
									EquipItem(item);
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;

		case ITEM_COSTUME:
#ifdef ENABLE_SYSTEM_RUNE
		case ITEM_RUNE:
		case ITEM_RUNE_RED:
		case ITEM_RUNE_BLUE:
		case ITEM_RUNE_GREEN:
		case ITEM_RUNE_YELLOW:
		case ITEM_RUNE_BLACK:
#endif
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:		// �ű� ���� ������
		case ITEM_BELT:		// �ű� ��Ʈ ������
			// MINING
		case ITEM_PICK:
			// END_OF_MINING
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
			// �������� ���� ��ȥ���� ����� �� ����.
			// �������� Ŭ����, ��ȥ���� ���Ͽ� item use ��Ŷ�� ���� �� ����.
			// ��ȥ�� ������ item move ��Ŷ���� �Ѵ�.
			// ������ ��ȥ���� �����Ѵ�.
		case ITEM_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::instance().PullOut(this, NPOS, item);
			break;
			}
		case ITEM_SPECIAL_DS:
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;

		case ITEM_FISH:
			{
#ifdef DISABLE_FISH_DECORUM
				if (CDecoredArenaManager::instance().IsArenaMap(GetMapIndex()) == true || CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
					return false;
				}
#else
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
					return false;
				}
#endif
#ifdef ENABLE_NEWSTUFF
				else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello.");
					return false;
				}
#endif

				if (item->GetSubType() == FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;
				//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("����� ��� �־ ������ �ʴ°� ����. ���踦 ���غ���."));
			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("����� ���� ������ �ƴѰ� ����."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("����� ������ ������ �ִ� �κ� ������ �ȵǾ����ϴ�."));
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
						ITEM_MANAGER::instance().RemoveItem(item);
						ITEM_MANAGER::instance().RemoveItem(item2);

						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
								#if defined(__CHATTING_WINDOW_RENEWAL__)
									ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
								#else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
								#endif
									break;
								case CSpecialItemGroup::EXP:
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ���� ���⸦ ���̸����� �����̴� �ӵ��� ���������ϴ�!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڰ� ���ڱ� �����Ͽ����ϴ�! ������� �����߽��ϴ�."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
									break;
#ifdef ENABLE_WOLFMAN_CHARACTER
								case CSpecialItemGroup::BLEEDING:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
									break;
#endif
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
									break;
								default:
									if (item_gets[i])
									{
#if defined(__CHATTING_WINDOW_RENEWAL__)
if (dwCounts[i] > 1)
	ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� %d �� ���Խ��ϴ�."), item_gets[i]->GetName(), dwCounts[i]);
else
	ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� ���Խ��ϴ�."), item_gets[i]->GetName());
#else
if (dwCounts[i] > 1)
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ڿ��� %s �� %d �� ���Խ��ϴ�."), item_gets[i]->GetName(), dwCounts[i]);
else
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ڿ��� %s �� ���Խ��ϴ�."), item_gets[i]->GetName());
#endif

									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("���谡 ���� �ʴ� �� ����."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("���谡 ���� �ʴ� �� ����."));
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			{
#ifdef ENABLE_NEWSTUFF
				if (0 != g_BoxUseTimeLimitValue)
				{
					if (get_dword_time() < m_dwLastBoxUseTime+g_BoxUseTimeLimitValue)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��带 ���� �� �����ϴ�."));
						return false;
					}
				}

				m_dwLastBoxUseTime = get_dword_time();
#endif
				DWORD dwBoxVnum = item->GetVnum();
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets(0);
				int count = 0;

				if( dwBoxVnum > 51500 && dwBoxVnum < 52000 )	// ��ȥ������
				{
					if( !(this->DragonSoul_IsQualified()) )
					{
						ChatPacket(CHAT_TYPE_INFO,LC_TEXT("���� ��ȥ�� ����Ʈ�� �Ϸ��ϼž� �մϴ�."));
						return false;
					}
				}

				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
				{
					item->SetCount(item->GetCount()-1);

					for (int i = 0; i < count; i++){
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
							break;
						case CSpecialItemGroup::EXP:
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ���� ���⸦ ���̸����� �����̴� �ӵ��� ���������ϴ�!"));
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڰ� ���ڱ� �����Ͽ����ϴ�! ������� �����߽��ϴ�."));
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
							break;
#ifdef ENABLE_WOLFMAN_CHARACTER
						case CSpecialItemGroup::BLEEDING:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
							break;
#endif
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
							break;
						default:
							if (item_gets[i])
							{
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� %d �� ���Խ��ϴ�."), item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� ���Խ��ϴ�."), item_gets[i]->GetName());
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("�ƹ��͵� ���� �� �������ϴ�."));
					return false;
				}
			}
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ų ������ �����µ� �����Ͽ����ϴ�."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ų ������ ���� �� �����ϴ�."));
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					// ���ο� ���ü��� value 0 �� ��ų ��ȣ�� �����Ƿ� �װ��� ���.
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum))
				{
#ifdef ENABLE_BOOKS_STACKFIX
					item->SetCount(item->GetCount() - 1);
#else
					ITEM_MANAGER::instance().RemoveItem(item);
#endif

					int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());

					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT :
						{
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC������ üũ�ؼ� ó��
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC���� �ƴ�!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� PC�濡���� ����� �� �ֽ��ϴ�."));
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi usare le pozioni durante un duello."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
										return false;
										break;
								}
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti durante il duello.");
								return false;
							}
#endif

							bool used = false;

							if (item->GetValue(0) != 0) // HP ���밪 ȸ��
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP ���밪 ȸ��
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % ȸ��
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % ȸ��
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �Ǵ� ���� �� ����Ͽ����ϴ�"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� �߿��� �̿��� �� ���� ��ǰ�Դϴ�."));
						return false;
					}
#ifdef ENABLE_NEWSTUFF
					else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
					{
						ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti durante il duello.");
						return false;
					}
#endif
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(GetLanguage()), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// �켱 ��ȥ���� ���ؼ��� �ϵ��� �Ѵ�.
							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%ld}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d�� ��ŭ �����Ǿ����ϴ�."), ret);
									item->SetCount(item->GetCount() - 1);
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%ld}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �� �����ϴ�."));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// �켱 ��ȥ���� ���ؼ��� �ϵ��� �Ѵ�.
							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d�� ��ŭ �����Ǿ����ϴ�."), ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �� �����ϴ�."));
									sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_SPECIAL:

						switch (item->GetVnum())
						{
							//ũ�������� ����
							case ITEM_NOG_POCKET:
								{
									/*
									���ִɷ�ġ : item_proto value �ǹ�
										�̵��ӵ�  value 1
										���ݷ�	  value 2
										����ġ    value 3
										���ӽð�  value 0 (���� ��)

									*/
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							//�󸶴ܿ� ����
							case ITEM_RAMADAN_CANDY:
								{
									/*
									�����ɷ�ġ : item_proto value �ǹ�
										�̵��ӵ�  value 1
										���ݷ�	  value 2
										����ġ    value 3
										���ӽð�  value 0 (���� ��)

									*/
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_RAMADAN_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
										return false;
									}
									// @fixme147 END
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									if (IsStillMarried())
									{
#ifdef ENABLE_DECORUM
										if (CArenaManager::instance().IsArenaMap(this->GetMapIndex()) == true || 
											CDecoredArenaManager::instance().IsArenaMap(this->GetMapIndex()) == true)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
											break;
										}
#else
										if (CArenaManager::instance().IsArenaMap(this->GetMapIndex()) == true)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello."));
											break;
										}
#endif
#ifdef ENABLE_DECORUM
										if(GetMarryPartner() != nullptr)
										{
											if (CArenaManager::instance().IsArenaMap(GetMarryPartner()->GetMapIndex()) == true || 
												CDecoredArenaManager::instance().IsArenaMap(GetMarryPartner()->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "You cannot use this item during a duel or decored arena."));
												break;
											}
										}

#else
										if(GetMarryPartner() != nullptr)
										{
											if (CArenaManager::instance().IsArenaMap(GetMarryPartner()->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "You cannot use this item during a duel."));
												break;
											}
										}
#endif
										WarpToPID(GetPartnerPID());
									}
									// else
										// ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ȥ ���°� �ƴϸ� ��ȥ������ ����� �� �����ϴ�."));
									
									// marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
									// if (pMarriage)
									// {
										// if (pMarriage->ch1 != NULL)
										// {
											// if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� �߿��� �̿��� �� ���� ��ǰ�Դϴ�."));
												// break;
											// }
										// }

										// if (pMarriage->ch2 != NULL)
										// {
											// if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											// {
												// ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� �߿��� �̿��� �� ���� ��ǰ�Դϴ�."));
												// break;
											// }
										// }

										// int consumeSP = CalculateConsumeSP(this);

										// if (consumeSP < 0)
											// return false;

										// PointChange(POINT_SP, -consumeSP, false);

										// WarpToPID(pMarriage->GetOther(GetPlayerID()));
									// }
									// else
										// ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ȥ ���°� �ƴϸ� ��ȥ������ ����� �� �����ϴ�."));
								}
								break;
							case 38060:
								{
									#ifdef ENABLE_EXPRESSING_EMOTION
										if(CountEmotion() >= 12){
											ChatPacket(CHAT_TYPE_INFO,"[Emotion] You can't put more emotions into it.");
											return false;
										}

										item->SetCount(item->GetCount()-1);

										int pct = number(1, 100);

										if (pct <= 70){
											InsertEmotion();
											ChatPacket(CHAT_TYPE_INFO,"[Emotion] Added emotion with success.");
										}

									#else
										ChatPacket(CHAT_TYPE_INFO,"[Emotion] Active.");
									#endif

								}
								break;

								//���� ����� ����
							case UNIQUE_ITEM_CAPE_OF_COURAGE:
								//�󸶴� ����� ����� ����
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
								AggregateMonster();
								//item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								// ���ָӴ�
								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									LuckyBagInfo * bi = NULL;
									bi = b1;

									int pct = number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										// ��ų���ü��� Ư���ϰ� �ش�.
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

							case 50004: // �̺�Ʈ�� ������
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										// ó�� ����
										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ������ �̺�Ʈ�� �����Ⱑ �������� �ʴ°� �����ϴ�."));
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{
										// �߰�!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̺�Ʈ�� �����Ⱑ �ź�ο� ���� ���� ������ϴ�."));

										// ���Ƚ���� ���� �ִ� �������� �ٸ��� �Ѵ�.
										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													if (info[i].dwVnum == 50001)
													{
														DWORD * pdw = M2_NEW DWORD[2];

														pdw[0] = info[i].dwVnum;
														pdw[1] = info[i].count;

														// ��÷���� ������ �����Ѵ�
														DBManager::instance().ReturnQuery(QID_LOTTO, GetPlayerID(), pdw,
																"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())",
																get_table_postfix(), GetPlayerID());
													}
													else
														AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ���ڱ��� ������

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									// ���� ��������� �������.
									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̺�Ʈ�� �����Ⱑ ������ ���� ������ϴ�."));
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ���ڱ��� ������

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989: // ����������
							case 76006: // ������ ����������
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												++stone;	//@fixme541
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����⸦ �ۿ��Ͽ����� �����Ǵ� ������ �����ϴ�."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����⸦ �ۿ��Ͽ����� �����Ǵ� ������ �����ϴ�."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											ITEM_MANAGER::instance().RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996: // ����
								item->SetCount(item->GetCount() - 1);
								AttackedByPoison(NULL); // @warme008
								break;
#ifdef PRIVATESHOP_SEARCH_SYSTEM
							case 60004:
								ChatPacket(CHAT_TYPE_COMMAND, "OpenShopSearch 1");
								break;

							case 60005:
								ChatPacket(CHAT_TYPE_COMMAND, "OpenShopSearch 2");
								break;
#endif
							case 27987: // ����
								// 50  ������ 47990
								// 30  ��
								// 10  ������ 47992
								// 7   û���� 47993
								// 3   ������ 47994
								{
									item->SetCount(item->GetCount() - 1);

									int r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� �������� ���Խ��ϴ�."));
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int * prob_table = prob_table_gb2312;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ������ ���� ������ϴ�."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� �����ְ� ���Խ��ϴ�."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� û���ְ� ���Խ��ϴ�."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�������� �����ְ� ���Խ��ϴ�."));
											AutoGiveItem(27994);
										}
									}
								}
								break;
#ifdef ENABLE_DECORUM
							case ITEM_NO_LOSS_DECORUM:
							case ITEM_GAIN_DECORUM:
							{
								if (GetDecorumArena() || (GetPvp() && GetPvp()->IsDecored()))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "You cannot use this item during a decorum event."));
									return false;
								}
								
								if (FindAffect(item->GetValue(0)))
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "L'effetto gia' sta funzionando."));
									return false;
								}
								
								AddAffect(item->GetValue(0), POINT_NONE, 0, 0, INFINITE_AFFECT_DURATION, 0, false);
								item->SetCount(item->GetCount() - 1);
							}
							break;
#endif
							case 71013: // ����������
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100: // ����
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200: // ������
								if (g_bEnableBootaryCheck)
								{
									if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
									{
										__OpenPrivateShop();
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ �� �� ���� �����Դϴ�"));
									}
								}
								else
								{
									__OpenPrivateShop();
								}
								break;

							case 50301: // ��ַ� ���ü�
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�а� �߿��� �ɷ��� �ø� �� �����ϴ�."));
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� å�� �ʹ� ����� �����ϱⰡ ����ϴ�."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� å�� �ƹ��� ���� ������ �� �� ���� �ʽ��ϴ�."));
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								{
#ifdef ENABLE_NEW_FISHING_SYSTEM
									if (FindAffect(AFFECT_FISH_MIND_PILL)) {
										ChatPacket(CHAT_TYPE_INFO, "TEST FISHING #MENTAL");
										return false;
									}
#endif

									AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 50304: // ����� ���ü�
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� 30�� �Ǳ� ������ ������ �� ���� �� ���� �ʽ��ϴ�."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� 50�� �Ǳ� ������ ������ �� ���� �� ���� �ʽ��ϴ�."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���̻� ������ �� �����ϴ�."));
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311: // ��� ���ü�
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� �Ϻ��ϰ� �˾Ƶ��� �� �ִ� ����̴�."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 : // �Ϻ� �� ��ȯ ��ų ���ü�
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316: // ���� ���ü�
							case 50323: case 50324: // ���� ���ü�
							case 50325: case 50326: // ö�� ���ü�
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�а� �߿��� �ɷ��� �ø� �� �����ϴ�."));
										return false;
									}

									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;

									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� å�� �������� ������ �� �÷��� �մϴ�."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� å�� �ʹ� ����� �����ϱⰡ ����ϴ�."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� å���δ� �� �̻� ������ �� �����ϴ�."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_CREATE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� ������ �� �����ϴ�."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����߿��� å�� ������ �����ϴ�."));
										return false;

									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �¸� ��ų�� ������ �� �ִ� ������ �ƴմϴ�."));
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											// �־ȼ��� ����߿��� �ð� ���� ����
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�־ȼ����� ���� ��ȭ�Ը����� �������Խ��ϴ�."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� �¸� ���ü��� ���� �� �����ϴ�."));
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�¸� ���ü��� �о� �¸� ��ų ����Ʈ�� ������ϴ�."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ����Ʈ�δ� �¸� ��ų�� ������ �ø� �� �ֽ��ϴ�."));
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�¸� ���ü� ���ؿ� �����Ͽ����ϴ�."));
									}
#ifdef ENABLE_BOOKS_STACKFIX
									item->SetCount(item->GetCount() - 1);
#else
									ITEM_MANAGER::instance().RemoveItem(item);
#endif
								}
								break;

							case 70102: // ����
							case 70103: // ����
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("������ �������±�. ������ �������� ���𰡰� �� �������� �����̾�."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����ġ�� %d �����Ͽ����ϴ�."), delta/10);
									}
								}
								break;

							case 71107: // õ��������
								{
									int val = item->GetValue(0);
									int interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
									int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ����� �� �����ϴ�."));
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�׽�Ʈ ���� �ð����� ���"));
										}
									}

									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����ġ�� �� �̻� �ø� �� �����ϴ�."));
										return false;
									}

									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}

									int old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);

									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("������ �������±�. ������ �������� ���𰡰� �� �������� �����̾�."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����ġ�� %d �����Ͽ����ϴ�."), val);

									char buf[256 + 1];
									snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
									LogManager::instance().CharLog(this, val, "MYTHICAL_PEACH", buf);
								}
								break;

							case 71109: // Ż����
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch( item2->GetType() )
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
											case ARMOR_EAR:
											case ARMOR_WRIST:
											case ARMOR_NECK:
#ifdef ENABLE_NEW_TALISMAN_GF
											case ARMOR_TALISMAN:
#ifdef ENABLE_NEW_TALISMAN_SLOTS
											case ARMOR_TALISMAN_2:
											case ARMOR_TALISMAN_3:
											case ARMOR_TALISMAN_4:
											case ARMOR_TALISMAN_5:
											case ARMOR_TALISMAN_6:
#endif											
#endif											
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ �����ϴ�"));
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int idx = ITEM_SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ �����ϴ�"));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)",
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::instance().ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:   // Ż����
							case 70202:   // ������(���)
							case 70203:   // ������(�ݻ�)
							case 70204:   // ������(������)
							case 70205:   // ������(����)
							case 70206:   // ������(������)
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��Ÿ�Ͽ����� ������ Ż���� �Ұ����մϴ�."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d ������ �Ǿ�� �ٽ� �����Ͻ� �� �ֽ��ϴ�."), last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;


									if (((item->GetVnum() == ITEM_VALENTINE_ROSE) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_VALENTINE_CHOCOLATE) && (SEX_FEMALE==GET_SEX(this))))
									{
										// ������ �����ʾ� �� �� ����.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����ʾ� �� �������� �� �� �����ϴ�."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;


									if (((item->GetVnum() == ITEM_WHITEDAY_CANDY) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_WHITEDAY_ROSE) && (SEX_FEMALE==GET_SEX(this))))
									{
										// ������ �����ʾ� �� �� ����.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����ʾ� �� �������� �� �� �����ϴ�."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case 50011: // ��������
								{
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::instance().ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_MONEY_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
												break;

											case CSpecialItemGroup::EXP:
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_EXP_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ڿ��� ���� �ź��� ���� ���ɴϴ�."));
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d�� ����ġ�� ȹ���߽��ϴ�."), dwCounts[i]);
#endif
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ���� ���⸦ ���̸����� �����̴� �ӵ��� ���������ϴ�!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڰ� ���ڱ� �����Ͽ����ϴ�! ������� �����߽��ϴ�."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
												break;
#ifdef ENABLE_WOLFMAN_CHARACTER
											case CSpecialItemGroup::BLEEDING:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���� ��� ���⸦ ���̸����� ���� �¸����� �����ϴ�!"));
												break;
#endif
											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� ���Ͱ� ��Ÿ�����ϴ�!"));
												break;

											default:
												if (item_gets[i])
												{
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� %d �� ���Խ��ϴ�."), item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ڿ��� %s �� ���Խ��ϴ�."), item_gets[i]->GetName());
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("�ƹ��͵� ���� �� �������ϴ�."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
#ifdef ENABLE_DECORUM
									if (CDecoredArenaManager::instance().IsArenaMap(GetMapIndex()) == true || CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
										return false;
									}
#else
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello."));
										return false;
									}
#endif
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello.");
										return false;
									}
#endif

									EffectPacket(SE_CHINA_FIREWORK);
#ifdef ENABLE_FIREWORK_STUN
									// ���� ������ �÷��ش�
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
#ifdef ENABLE_DECORUM
									if (CDecoredArenaManager::instance().IsArenaMap(GetMapIndex()) == true || CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello eroe o un arena."));
										return false;
									}
#else
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzare questo oggetto durante un duello."));
										return false;
									}
#endif
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello.");
										return false;
									}
#endif

									EffectPacket(SE_SPIN_TOP);
#ifdef ENABLE_FIREWORK_STUN
									// ���� ������ �÷��ش�
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case 90008: // VCARD
							case 90009: // VCARD
								VCardUse(this, this, item);
								break;

							case ITEM_ELK_VNUM: // ���ٷ���
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� %d ���� ȹ���߽��ϴ�."), iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;
								

#ifdef __BATTLE_PASS__
							case ITEM_BATTLE_PASS: 
								{
									if (!v_counts.empty())
									{
										ChatPacket(CHAT_TYPE_INFO, "You have already one active!");
										return false;
									}
									
									FILE 	*fileID;
									char file_name[256+1];

									snprintf(file_name, sizeof(file_name), "%s/battlepass_players/%s.txt", LocaleService_GetBasePath().c_str(),GetName());
									fileID = fopen(file_name, "w");
									
									if (NULL == fileID)
										return false;

									for (int i=0; i<missions_bp.size(); ++i)
									{
										fprintf(fileID,"MISSION	%d	%d\n", 0, 0);
									}
									
									fclose(fileID);

									Load_BattlePass();
									ChatPacket(CHAT_TYPE_INFO, "You activate battle pass for this month!");
									item->SetCount(item->GetCount() - 1);
								}
								break;
#endif

#ifdef BYKATIL199_ITEM_SLOT_EFFECT
							case NEW_MOVE_SPEED_POTION:
							case NEW_ATTACK_SPEED_POTION:
								{
									EAffectTypes type = AFFECT_NONE;

									if (item->GetVnum() == NEW_MOVE_SPEED_POTION)
										type = AFFECT_MOV_SPEED;

									if (item->GetVnum() == NEW_ATTACK_SPEED_POTION)
										type = AFFECT_ATT_SPEED;

									if (AFFECT_NONE == type)
										break;

									CAffect * pAffect = FindAffect(type);

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;
										EAffectBits flag = AFF_NONE;

										if (item->GetVnum() == NEW_MOVE_SPEED_POTION)
										{
											bonus = POINT_MOV_SPEED;
											flag = AFF_MOV_SPEED_POTION;
										}

										if (item->GetVnum() == NEW_ATTACK_SPEED_POTION)
										{
											bonus = POINT_ATT_SPEED;
											flag = AFF_ATT_SPEED_POTION;
										}

										AddAffect(type, bonus, item->GetValue(2), flag, INFINITE_AFFECT_DURATION, 0, true);

										item->Lock(true);
										item->SetSocket(0, true);
									}
									else
									{
										RemoveAffect(pAffect);
										item->Lock(false);
										item->SetSocket(0, false);
									}
								}
								break;
							case NEW_KRITIK_POTION:
							case NEW_DELICI_POTION:
							case NEW_DRAGON_1_POTION:
							case NEW_DRAGON_2_POTION:
							case NEW_DRAGON_3_POTION:
							case NEW_DRAGON_4_POTION:
								{
									EAffectTypes type = AFFECT_NONE;

									if (item->GetVnum() == NEW_KRITIK_POTION)
										type = AFFECT_NEW_AFFECT_POTION_1;

									if (item->GetVnum() == NEW_DELICI_POTION)
										type = AFFECT_NEW_AFFECT_POTION_2;

									if (item->GetVnum() == NEW_DRAGON_1_POTION)
										type = AFFECT_NEW_AFFECT_POTION_3;

									if (item->GetVnum() == NEW_DRAGON_2_POTION)
										type = AFFECT_NEW_AFFECT_POTION_4;

									if (item->GetVnum() == NEW_DRAGON_3_POTION)
										type = AFFECT_NEW_AFFECT_POTION_5;

									if (item->GetVnum() == NEW_DRAGON_4_POTION)
										type = AFFECT_NEW_AFFECT_POTION_6;

									if (AFFECT_NONE == type)
										break;

									CAffect * pAffect = FindAffect(type);

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;
										EAffectBits flag = AFF_NONE;

										if (item->GetVnum() == NEW_KRITIK_POTION)
										{
											bonus = POINT_CRITICAL_PCT;
										}

										if (item->GetVnum() == NEW_DELICI_POTION)
										{
											bonus = POINT_PENETRATE_PCT;
										}

										if (item->GetVnum() == NEW_DRAGON_1_POTION)
										{
											bonus = POINT_STEAL_HP;
										}

										if (item->GetVnum() == NEW_DRAGON_2_POTION)
										{
											bonus = POINT_ATT_BONUS;
										}

										if (item->GetVnum() == NEW_DRAGON_3_POTION)
										{
											bonus = POINT_STEAL_SP;
										}

										if (item->GetVnum() == NEW_DRAGON_4_POTION)
										{
											bonus = POINT_DEF_BONUS;
										}

										AddAffect(type, bonus, item->GetValue(2), flag, INFINITE_AFFECT_DURATION, 0, true);

										item->Lock(true);
										item->SetSocket(0, true);
									}
									else
									{
										RemoveAffect(pAffect);
										item->Lock(false);
										item->SetSocket(0, false);
									}
								}
								break;
							case NEW_SEBNEM_PEMBE:
							case NEW_SEBNEM_KIRMIZI:
							case NEW_SEBNEM_MAVI:
							case NEW_SEBNEM_BEYAZ:
							case NEW_SEBNEM_YESIL:
							case NEW_SEBNEM_SARI:
								{
									EAffectTypes type = AFFECT_NONE;

									if (item->GetVnum() == NEW_SEBNEM_PEMBE)
										type = AFFECT_NEW_SEBNEM_POTION_1;

									if (item->GetVnum() == NEW_SEBNEM_KIRMIZI)
										type = AFFECT_NEW_SEBNEM_POTION_2;

									if (item->GetVnum() == NEW_SEBNEM_MAVI)
										type = AFFECT_NEW_SEBNEM_POTION_3;

									if (item->GetVnum() == NEW_SEBNEM_BEYAZ)
										type = AFFECT_NEW_SEBNEM_POTION_4;

									if (item->GetVnum() == NEW_SEBNEM_YESIL)
										type = AFFECT_NEW_SEBNEM_POTION_5;

									if (item->GetVnum() == NEW_SEBNEM_SARI)
										type = AFFECT_NEW_SEBNEM_POTION_6;

									if (AFFECT_NONE == type)
										break;

									CAffect * pAffect = FindAffect(type);

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;
										EAffectBits flag = AFF_NONE;

										if (item->GetVnum() == NEW_SEBNEM_PEMBE)
										{
											bonus = POINT_PENETRATE_PCT;
										}

										if (item->GetVnum() == NEW_SEBNEM_KIRMIZI)
										{
											bonus = POINT_CRITICAL_PCT;
										}

										if (item->GetVnum() == NEW_SEBNEM_MAVI)
										{
											bonus = POINT_ATT_GRADE_BONUS;
										}

										if (item->GetVnum() == NEW_SEBNEM_BEYAZ)
										{
											bonus = POINT_DEF_GRADE_BONUS;
										}

										if (item->GetVnum() == NEW_SEBNEM_YESIL)
										{
											bonus = POINT_RESIST_MAGIC;
										}

										if (item->GetVnum() == NEW_SEBNEM_SARI)
										{
											bonus = POINT_ATT_SPEED;
										}

										AddAffect(type, bonus, item->GetValue(2), flag, INFINITE_AFFECT_DURATION, 0, true);

										item->Lock(true);
										item->SetSocket(0, true);
									}
									else
									{
										RemoveAffect(pAffect);
										item->Lock(false);
										item->SetSocket(0, false);
									}
								}
								break;
#endif

								//������ ��ǥ
							case 70021:
								{
									int HealPrice = quest::CQuestManager::instance().GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::instance().HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), LC_TEXT("������ �ູ���� ������ %s ������ HP,SP�� ��� ä�����ϴ�."), EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ູ�� ����Ͽ����ϴ�."));
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 : // ���� ��ü�� �ӽ�
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

							case 71051 : // ���簡
								{
									// ����, �̰���, ��Ʈ�� ���簡 ������
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (ITEM_COSTUME == item2->GetType()) // @fixme124
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}
									
									if (item2->GetVnum() == 79501)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu mai puteti adauga bonusuri."));
										return false;
									}

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}

									if (item2->AddRareAttribute() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���������� �Ӽ��� �߰� �Ǿ����ϴ�"));

										int iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� �� ���������� �Ӽ��� �߰��� �� �����ϴ�"));
									}
								}
								break;

							case 71052 : // �����
								{
									// ����, �̰���, ��Ʈ�� ���簡 ������
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (ITEM_COSTUME == item2->GetType()) // @fixme124
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}

									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��ų �Ӽ��� �����ϴ�"));
									}
								}
								break;

							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:
							// ���ù��������� ������ �ϴ� �� ��ġ��� ������...
							// �׷��� �׳� �ϵ� �ڵ�. ���� ���ڿ� �ڵ����� �����۵�.
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
#ifdef ENABLE_DECORUM
									CDecoredArena * pkArenaMap = CDecoredArenaManager::instance().FindDecoredArena(GetMapIndex());
									if (pkArenaMap != nullptr && pkArenaMap->GetType() < ARENA3 && 
										quest::CQuestManager::instance().GetEventFlag("arena_auto_potion") == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi utilizzarlo nell'Arena"));
										return false;
									}
#endif									
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
										return false;
									}
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello." );
										return false;
									}
#endif

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����ǰ�� �� ������ �����ϴ�."));
											break;
										}

										item->SetCount( item->GetCount() - 1 );

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect( type );

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess( type );
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect( pAffect );

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID( pAffect->dwFlag );

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect( pAffect );

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess( type );
										}
									}
								}
								break;
						}
						break;

					case USE_CLEAR:
						{
							switch (item->GetVnum())
							{
#ifdef ENABLE_WOLFMAN_CHARACTER
								case 27124: // Bandage
									RemoveBleeding();
									break;
#endif
								case 27874: // Grilled Perch
								default:
									RemoveBadAffect();
									break;
							}
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ����� �� �����ϴ�."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi usare questi potenziamenti in duello."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
										return false;
								}
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello." );
								return false;
							}
#endif

							bool used = false;

							if (item->GetValue(0) != 0) // HP ���밪 ȸ��
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP ���밪 ȸ��
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % ȸ��
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % ȸ��
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �Ǵ� ���� �� ����Ͽ����ϴ�"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
								return false;
							}

							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi usare le pozioni durante un duello."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����忡�� ����Ͻ� �� �����ϴ�."));
									return false;
							}
						}
#ifdef ENABLE_NEWSTUFF
						else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
						{
							ChatPacket(CHAT_TYPE_INFO, "Non puoi usare miglioramenti in duello." );
							return false;
						}
#endif

						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
#ifdef UNLIMITED_GREEN_PURPLE_P_A_C
										if (FindAffect(AFFECT_MOV_SPEED))
											ChatPacket(CHAT_TYPE_INFO, "Devi aspettare per usare di nuovo questa pozione.");
										else
											AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#else
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#endif
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_DXUP_PURPLE);
#endif
									break;

								case APPLY_ATT_SPEED:
#ifdef UNLIMITED_GREEN_PURPLE_P_A_C
										if (FindAffect(AFFECT_ATT_SPEED))
											ChatPacket(CHAT_TYPE_INFO, "Devi aspettare per usare di nuovo questa pozione.");
										else
											AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#else
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#endif
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_SPEEDUP_GREEN);
#endif
									break;

								case APPLY_STR:
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);
#ifdef UNLIMITED_GREEN_PURPLE_P_A_C
							if (item->GetVnum() != 27102 && item->GetVnum() != 27105)
								item->SetCount(item->GetCount() - 1);
#else
						item->SetCount(item->GetCount() - 1);
#endif
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;

#ifdef ENABLE_DECORUM
							if (CDecoredArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(), "Non puoi usare questo oggetto durante un duello o un arena."));
								return false;
							}
#endif
							// gm_guild_build, oxevent �ʿ��� ��ȯ�� ��ȯ���� �� �����ϰ� ����
							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��ġ���� ����� �� �����ϴ�."));
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��� �߿��� �̿��� �� ���� ��ǰ�Դϴ�."));
								return false;
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "You can't do this during the fight." );
								return false;
							}
#endif

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̵��� �غ� �Ǿ��������� ��ȯ�θ� ����Ҽ� �����ϴ�"));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL) // ��ȯ��
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��ġ�� ����"));

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL) // ��ȯ����
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �ȿ����� %s%s ����� �� �����ϴ�."),
												item->GetName(GetLanguage()),
												"");
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
							if (item->GetValue(0) == ACCE_CLEAN_ATTR_VALUE0)
							{
								if (!CleanAcceAttr(item, item2))
									return false;
								return true;
							}
#endif
#ifdef CHANGELOOK_SYSTEM
							if (item->GetValue(0) == CL_CLEAN_ATTR_VALUE0)
							{
								if (!CleanTransmutation(item, item2))
									return false;

								return true;
							}
#endif
							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343) // ����+3
							{
								if (item->GetVnum() >= 25040 && item->GetVnum() <= 25041) // û���Ǽ���
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi effettuare l'operazione."));
								}
							}

							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28643)  // ����+4
							{
								if (item->GetVnum() >= 25040 && item->GetVnum() <= 25041) // û���Ǽ���
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi effettuare l'operazione."));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

					case USE_CHANGE_COSTUME_ATTR:
					case USE_RESET_COSTUME_ATTR:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
							if (ITEM_COSTUME != item2->GetType() || item2->GetSubType() == COSTUME_MOUNT)
#else
							if (ITEM_COSTUME != item2->GetType())
#endif
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
								return false;
							}

							if (item2->GetAttributeCount() == 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �Ӽ��� �����ϴ�."));
								return false;
							}

							switch (item->GetSubType())
							{
								case USE_CHANGE_COSTUME_ATTR:
								#ifndef ENABLE_USE_DIFFERENT_TABLE_FOR_COSTUME_ATTRIBUTE
									item2->ChangeAttribute();
								#else
									item2->ChangeCostumeAttribute();
								#endif
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_COSTUME_ATTR", buf);
									}
									break;
								case USE_RESET_COSTUME_ATTR:
									item2->ClearAttribute();
									item2->AlterToMagicItem();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "RESET_COSTUME_ATTR", buf);
									}
									break;
							}

							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai cambiato i bonus con successo."));

							item->SetCount(item->GetCount() - 1);
							break;
						}
 #ifdef CHANGE_PETSYSTEM
					case USE_CHANGE_LV_P:
					{
						LPITEM item2;
						if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
							return false;

						if (item2->IsEquipped())
						{
							BuffOnAttr_RemoveBuffsFromItem(item2);
						}

						if (ITEM_COSTUME == item2->GetType())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("?O???�� ??��?C? ?o ??��A ???????O��?��?."));
							return false;
						}

						if (item2->IsExchanging() || item2->IsEquipped())
							return false;
						if (item2->GetVnum() == 55701 || item2->GetVnum() == 55702 || item2->GetVnum() == 55703 || item2->GetVnum() == 55704 || item2->GetVnum() == 55705 || item2->GetVnum() == 55706 || item2->GetVnum() == 55716)
						{
							if(GetNewPetSystem()->IsActivePet()){
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Trebuie intai sa alungati petul."));
								return false;
							}
							else{
								int pet_level = item2->GetSocket(1);
	
								int new_hp = number(1,23);
								int new_cs = number(1,23);
								int new_av = number(1,23);
								size_t new_hp_b = 0;
								for(int i = 0; i < (pet_level / 5); i++){
									new_hp_b += number(1,6);
								}
								size_t new_cs_b = 0;
								for(int i = 0; i < (pet_level / 7); i++){
									new_cs_b += number(1,6);
								}
								size_t new_av_b = 0;
								for(int i = 0; i < (pet_level / 4); i++){
									new_av_b += number(1,6);
								}
								int new_hp_d = new_hp + new_hp_b;
								int new_cs_d = new_cs + new_cs_b;
								int new_av_d = new_av + new_av_b;
	
								DBManager::instance().DirectQuery("UPDATE new_petsystem SET bonus0=%d, bonus1=%d, bonus2=%d WHERE id = %lu ", new_hp_d, new_cs_d, new_av_d, item2->GetID());
								if (item2 != NULL){
									item2->SetForceAttribute(0, 1, new_hp_d);
									item2->SetForceAttribute(1, 1, new_cs_d);
									item2->SetForceAttribute(2, 1, new_av_d);
								}
	
								int skill_slots = number(1,3);
								int enabled_skill = 0;
								int blocked_skill = -1;
								int default_skilllv = 0;
								if (skill_slots == 1){
									DBManager::instance().DirectQuery("UPDATE new_petsystem SET skill0=%d, skill0lv= %d, skill1=%d, skill1lv= %d, skill2=%d, skill2lv= %d WHERE id = %lu ", enabled_skill, default_skilllv, blocked_skill, default_skilllv, blocked_skill, default_skilllv, item2->GetID());
								}
								else if (skill_slots == 2){
									DBManager::instance().DirectQuery("UPDATE new_petsystem SET skill0=%d, skill0lv= %d, skill1=%d, skill1lv= %d, skill2=%d, skill2lv= %d WHERE id = %lu ", enabled_skill, default_skilllv, enabled_skill, default_skilllv, blocked_skill, default_skilllv, item2->GetID());
								}
								else if (skill_slots == 3){
									DBManager::instance().DirectQuery("UPDATE new_petsystem SET skill0=%d, skill0lv= %d, skill1=%d, skill1lv= %d, skill2=%d, skill2lv= %d WHERE id = %lu ", enabled_skill, default_skilllv, enabled_skill, default_skilllv, enabled_skill, default_skilllv, item2->GetID());
								}
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai schimbat bonusul."));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Slot deblocat : %d."), skill_slots);
								item->SetCount(item->GetCount() - 1);
								break;
							}
						}
						else{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Change Pet Bonus@"));
							return false;
						}
					}
					break;
#endif
						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_BELT_SOCKET:
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:
					case USE_CHANGE_ATTRIBUTE2:
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;
							
							
							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}
#ifdef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
							if ((item2->GetType() == ITEM_DS && item->GetVnum() != 71097) || (item2->GetType() != ITEM_DS && item->GetVnum() == 71097))
							{
								ChatPacket(CHAT_TYPE_INFO, "You cannot change bonuses with this item");
								return false;
							}
#endif
							// [NOTE] �ڽ�Ƭ �����ۿ��� ������ ���� ������ ���� �Ӽ��� �ο��ϵ�, ����簡 ����� ���ƴ޶�� ��û�� �־���.
							// ���� ANTI_CHANGE_ATTRIBUTE ���� ������ Flag�� �߰��Ͽ� ��ȹ �������� �����ϰ� ��Ʈ�� �� �� �ֵ��� �� �����̾�����
							// �׵��� �ʿ������ ��ġ�� ���� �ش޷��� �׳� ���⼭ ����... -_-
							if (ITEM_COSTUME == item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
								return false;
							}

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;
						
							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										int i;
										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM_SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"û���� ���� �������� �ʽ��ϴ�."));
											return false;
										}

										int j = 0;

										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM_SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM

								case USE_CHANGE_ATTRIBUTE2:
								{
									// ??, ???, ??? ??? ????
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (ITEM_COSTUME == item2->GetType()) // @fixme124
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("??? ??? ? ?? ??????."));
										return false;
									}

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("??? ??? ? ?? ??????."));
										return false;
									}
#ifdef ENABLE_SOULBIND_SYSTEM
									if (item2->IsSealed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't do this because this item is binded."));
										return false;
									}
#endif
									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);
										#ifdef ENABLE_REAL_TIME_ENCHANT
										if (item->GetVnum() != 71151 && item->GetSocket(0) == 0)
										#endif
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("?? ?? ??? ????"));
									}
								}
								break;

#endif
								case USE_CHANGE_ATTRIBUTE :
								//case USE_CHANGE_ATTRIBUTE2 : // @fixme123
									#ifdef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
										if (item2->GetAttributeSetIndex() == -1 && item2->GetType() != ITEM_DS)
									#else
										if (item2->GetAttributeSetIndex() == -1)
									#endif
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}

									#ifdef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
										if (item2->GetAttributeCount() == 0 && item2->GetType() != ITEM_DS)
									#else
										if (item2->GetAttributeCount() == 0)
									#endif
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �Ӽ��� �����ϴ�."));
										return false;
									}

									if ((GM_PLAYER == GetGMLevel()) && (false == test_server) && (g_dwItemBonusChangeTime > 0))
									{
										//
										// Event Flag �� ���� ������ ������ �Ӽ� ������ �� �ð����� ���� ����� �ð��� �귶���� �˻��ϰ�
										// �ð��� ����� �귶�ٸ� ���� �Ӽ����濡 ���� �ð��� ������ �ش�.
										//

										// DWORD dwChangeItemAttrCycle = quest::CQuestManager::instance().GetEventFlag(msc_szChangeItemAttrCycleFlag);
										// if (dwChangeItemAttrCycle < msc_dwDefaultChangeItemAttrCycle)
											// dwChangeItemAttrCycle = msc_dwDefaultChangeItemAttrCycle;
										DWORD dwChangeItemAttrCycle = g_dwItemBonusChangeTime;

										quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

										if (pPC)
										{
											DWORD dwNowSec = get_global_time();

											DWORD dwLastChangeItemAttrSec = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

											if (dwLastChangeItemAttrSec + dwChangeItemAttrCycle > dwNowSec)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� �ٲ��� %d�� �̳����� �ٽ� ������ �� �����ϴ�.(%d �� ����)"),
														dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowSec - dwLastChangeItemAttrSec));
												return false;
											}

											pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowSec);
										}
									}

									if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 0, 30, 40, 3
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else if (item->GetVnum() == 76014)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 10, 50, 39, 1
										};

										item2->ChangeAttribute(aiChangeProb);
									}

									else
									{
										// ??? ????
										// ??? ??? ?? ???? ?? ?? ???.
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
											if ((item2->GetType() == ITEM_WEAPON) || (item2->GetType() == ITEM_ARMOR))

											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 50)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �������� ���� ����� �Ұ����մϴ�."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����� ���ʿ��� ��� �����մϴ�."));
												break;
											}
										}
																		
										
#ifndef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
										item2->ChangeAttribute();
#endif	
									}

									#ifdef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
										if (item2->GetType() == ITEM_DS && item->GetVnum() == 71097)
										{
											if(DSManager::instance().ChangeAttributes(this,item2)){
												item->SetCount(item->GetCount() - 1);
											}
											break;
										}

										else{

											item2->ChangeAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai schimbat cu succes bonusurile."));
											{
												char buf[21];
												snprintf(buf, sizeof(buf), "%u", item2->GetID());
												LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
											}
											#ifdef ENABLE_REAL_TIME_ENCHANT
											if (item->GetVnum() != 71151 && item->GetSocket(0) == 0)
											#endif
											item->SetCount(item->GetCount() - 1);									
											break;
										}
									#else

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai schimbat cu succes bonusurile."));
										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
										}
										#ifdef ENABLE_REAL_TIME_ENCHANT
										if (item->GetVnum() != 71151 && item->GetSocket(0) == 0)
										#endif
										item->SetCount(item->GetCount() - 1);

									#endif
									break;

								case USE_ADD_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}
									
									if (item2->GetVnum() == 79501)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu mai puteti adauga bonusuri."));
										return false;
									}

									if (item2->GetAttributeCount() < 5)
									{
										// ���簡 Ư��ó��
										// ����� ���簡 �߰� �ȵɰŶ� �Ͽ� �ϵ� �ڵ���.
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
#ifdef ENABLE_NEW_TALISMAN_GF
											if ((item2->GetType() == ITEM_WEAPON)
												|| ((item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN)
#ifdef ENABLE_NEW_TALISMAN_SLOTS
												 || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN_2) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN_3)
												 || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN_4) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN_5)
												  || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_TALISMAN_6)
#endif
												))
#else
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
#endif
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 50)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"TEST1"));
													break;
												}
											}
											/*else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"TEST2."));
												break;
											}*/
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 1) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											#ifdef ENABLE_INSTANT_BONUS
											for (int i = 0; i < 5 && item2->GetAttributeCount() < 5; i++)
											{
												item2->AddAttribute();
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ� �߰��� �����Ͽ����ϴ�."));

												int iAddedIdx = item2->GetAttributeCount() - 1;
												LogManager::instance().ItemLog(GetPlayerID(), item2->GetAttributeType(iAddedIdx), item2->GetAttributeValue(iAddedIdx),
														item->GetID(), "ADD_ATTRIBUTE_SUCCESS", buf, GetDesc()->GetHostName(), item->GetOriginalVnum());
											}
											#else
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ� �߰��� �����Ͽ����ϴ�."));
											
											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(GetPlayerID(), item2->GetAttributeType(iAddedIdx), item2->GetAttributeValue(iAddedIdx),
													item->GetID(), "ADD_ATTRIBUTE_SUCCESS", buf, GetDesc()->GetHostName(), item->GetOriginalVnum());
												#endif
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ� �߰��� �����Ͽ����ϴ�."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE_FAIL", buf);
										}

									//	item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���̻� �� �������� �̿��Ͽ� �Ӽ��� �߰��� �� �����ϴ�."));
									}
									break;

								case USE_ADD_ATTRIBUTE2 :
									// �ູ�� ����
									// �簡�񼭸� ���� �Ӽ��� 4�� �߰� ��Ų �����ۿ� ���ؼ� �ϳ��� �Ӽ��� �� �ٿ��ش�.
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ��� ������ �� ���� �������Դϴ�."));
										return false;
									}
									
									if (item2->GetVnum() == 79501)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu mai puteti adauga bonusuri."));
										return false;
									}

									// �Ӽ��� �̹� 4�� �߰� �Ǿ��� ���� �Ӽ��� �߰� �����ϴ�.
									if (item2->GetAttributeCount() == 4)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 1) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ� �߰��� �����Ͽ����ϴ�."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE2_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�Ӽ� �߰��� �����Ͽ����ϴ�."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE2_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �̻� �� �������� �̿��Ͽ� �Ӽ��� �߰��� �� �����ϴ�."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �簡�񼭸� �̿��Ͽ� �Ӽ��� �߰����� �ּ���."));
									}
									else
									{
										// wtf ?!
										sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d)", item2->GetAttributeCount());
									}
									break;

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
#ifdef ENABLE_ADDSTONE_FAILURE
												if (number(1, 100) <= 40)
#else
												if (1)
#endif
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���������� �߰��Ǿ����ϴ�."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �߰��� �����Ͽ����ϴ�."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �׼��������� ���̻� ������ �߰��� ������ �����ϴ�."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ���������� ������ �߰��� �� ���� �������Դϴ�."));
										}
									}
									break;

								case USE_PUT_INTO_BELT_SOCKET:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
										{
											if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
											{
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����Ͽ����ϴ�."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����Ͽ����ϴ�."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
											}

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���̾Ƹ��� �Ǽ������� ������ �߰��ؾ��մϴ�."));
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �׼��������� ���̻� ������ ������ �����ϴ�."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���̾Ƹ��� ������ �߰��ؾ��մϴ�."));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �׼��������� ���̻� ������ ������ �� �����ϴ�."));
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
									}
									break;
							}
							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{

							if (m_pkFishingEvent || m_pkFishingNewEvent) //ENABLE_NEW_FISHING_SYSTEM
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �߿� �̳��� ���Ƴ��� �� �����ϴ�."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� �����ִ� �̳��� ���� %s�� ����ϴ�."), item->GetName(GetLanguage()));
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���ô뿡 %s�� �̳��� ����ϴ�."), item->GetName(GetLanguage()));
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT :
						{
							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC������ üũ�ؼ� ó��
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC���� �ƴ�!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� PC�濡���� ����� �� �ֽ��ϴ�."));
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					// ���� ���� ��ų�� ������ ó��
					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ���� ��ᰡ �����մϴ�."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ���� ��ᰡ �����մϴ�."));
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���(%s)�� �����մϴ�."), pSource1->GetName());
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���(%s)�� �����մϴ�."), pSource2->GetName());
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ���� ���ڸ��ϴ�."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ �����߽��ϴ�."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				int i;

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ��ƾ���� ������ ������ �� �����ϴ�."));
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ��ƾ���� ��� ������ �� �����ϴ�."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� ��ƾ���� ���⿡ ������ �� �����ϴ�."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �� �ִ� ������ �����ϴ�."));
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
						// �� Ȯ��
#ifdef ENABLE_ADDSTONE_FAILURE
						if (number(1, 100) <= 40)
#else
						if (1)
#endif
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ƾ�� ������ �����Ͽ����ϴ�."));
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ƾ�� ������ �����Ͽ����ϴ�."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::instance().ItemLog(this, item2, "SOCKET", item->GetName(GetLanguage()));
						
						#ifdef FIX_STACKABLE_ITEM
						item->SetCount(item->GetCount() - 1);
						#else
						ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (METIN)");
						#endif
						break;
					}

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �� �ִ� ������ �����ϴ�."));
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM_BLEND:
			// ���ο� ���ʵ�
			sys_log(0,"ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;
				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2);

				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ȿ���� �ɷ� �ֽ��ϴ�."));
					}
					else
					{
						#ifdef ENABLE_NEW_AFFECT_POTION
						SetAffectPotion(item);
						#endif
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
					}
				}
			}
			break;

		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName(GetLanguage()));
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(GetLanguage()), item->GetType());
			return false;
	}

	return true;
}

int g_nPortalLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;

	//WORD wDestCell = DestCell.cell;
	//BYTE bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
			return false;

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(GetLanguage()), window_type, wCell);

	if (item->IsExchanging())
		return false;
	
#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Cannot remove item from switchbot. Inventory is full."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif	

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����ʾ� �� �������� ����� �� �����ϴ�."));
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����ʾ� �� �������� ����� �� �����ϴ�."));
		return false;
	}

#ifdef FAST_EQUIP_WORLDARD
	if(Cell.IsChangeEquipPosition())
	{
		int iEmptyCell = GetEmptyInventory(item->GetSize());
		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu puteti retrage itemele din FastEquip. Inventarul este full."));
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}

#endif

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"����Ҽ� �����ϴ�."));
			return false;
		}

		// ��ȥ���� ����� ������ SUMMONABLE_ZONE�� �ִ°��� WarpToPC()���� üũ

		//��Ÿ� ���� �ʿ����� ��ȯ�θ� ���ƹ�����.
		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��Ÿ� ���� �����߿��� ��ȯ��,��ȯ���θ� ����Ҽ� �����ϴ�."));
			return false;
		}
		int iPulse = thecore_pulse();

		//â�� ���� üũ
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"â�� ���� %d�� �̳����� ��ȯ��,��ȯ���θ� ����� �� �����ϴ�."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false;
		}

		//�ŷ����� â üũ
#ifdef OFFLINE_SHOP
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner())
#else
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
#endif
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ŷ�â,â�� ���� �� ���¿����� ��ȯ��,��ȯ���� �� ����Ҽ� �����ϴ�."));
			return false;
		}

		//PREVENT_REFINE_HACK
		//������ �ð�üũ
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ������ %d�� �̳����� ��ȯ��,��ȯ���θ� ����� �� �����ϴ�."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK

#if defined(__BL_MAILBOX__)
		{
			if (iPulse - GetMyMailBoxTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, "You cannot use a Return Scroll %d seconds after opening a mailbox.", g_nPortalLimitTime);
				return false;
			}
		}
#endif
		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���λ��� ����� %d�� �̳����� ��ȯ��,��ȯ���θ� ����� �� �����ϴ�."), g_nPortalLimitTime);
				return false;
			}
#ifdef OFFLINE_SHOP
			if (iPulse - GetMyOfflineShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���λ��� ����� %d�� �̳����� ��ȯ��,��ȯ���θ� ����� �� �����ϴ�."), g_nPortalLimitTime);
				return false;
			}
#endif
		}
		//END_PREVENT_ITEM_COPY


		//��ȯ�� �Ÿ�üũ
		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;
			//��ȯ����
			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}
			//��ȯ��
			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̵� �Ǿ��� ��ġ�� �ʹ� ����� ��ȯ�θ� ����Ҽ� �����ϴ�."));
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist);
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE
		//��ȯ �� �ð�üũ
		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ŷ� �� %d�� �̳����� ��ȯ��,��ȯ���ε��� ����� �� �����ϴ�."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	//������ ��� ���� �ŷ�â ���� üũ
	if ((item->GetVnum() == 50200) || (item->GetVnum() == 71049))
	{
#ifdef OFFLINE_SHOP
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
#else
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
#endif
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare questo oggetto mentre stai commerciando.");
			return false;
		}

	}

#ifdef ITEM_BUFF_SYSTEM
	if (item->GetVnum() == MASTER_RESIST_BLESS_ITEM_BUFF_VNUM)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_RESIST_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_HOSIN))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}		
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_RESIST_BUFF, POINT_RESIST_NORMAL_DAMAGE, MASTER_RESIST_BLESS_AFFECT_VALUE, AFF_RESIST_BUFF, MASTER_RESIST_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= MASTER_RESIST_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Benedizione per 160 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 24%.");			
		}			
	}
}

	if (item->GetVnum() == GRAND_RESIST_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_RESIST_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_HOSIN))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_RESIST_BUFF, POINT_RESIST_NORMAL_DAMAGE, GRAND_RESIST_BLESS_AFFECT_VALUE, AFF_RESIST_BUFF, GRAND_RESIST_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= GRAND_RESIST_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Benedizione per 224 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 29%.");			
		}			
	}
}

	if (item->GetVnum() == PERFECT_RESIST_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_RESIST_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_HOSIN))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Benedizione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}		
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_RESIST_BUFF, POINT_RESIST_NORMAL_DAMAGE, PERFECT_RESIST_BLESS_AFFECT_VALUE, AFF_RESIST_BUFF, PERFECT_RESIST_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= PERFECT_RESIST_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Benedizione per 310 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 35%.");			
		}			
	}
}

	if (item->GetVnum() == MASTER_CRITICAL_BLESS_ITEM_BUFF_VNUM)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_CRITICAL_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_GICHEON))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}			
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_CRITICAL_BUFF, POINT_CRITICAL_PCT, MASTER_CRITICAL_BLESS_AFFECT_VALUE, AFF_CRITICAL_BUFF, MASTER_CRITICAL_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= MASTER_CRITICAL_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Aiuto del drago per 110 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 24%.");			
		}			
	}
}

	if (item->GetVnum() == GRAND_CRITICAL_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_CRITICAL_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_GICHEON))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_CRITICAL_BUFF, POINT_CRITICAL_PCT, GRAND_CRITICAL_BLESS_AFFECT_VALUE, AFF_CRITICAL_BUFF, GRAND_CRITICAL_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= GRAND_CRITICAL_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Aiuto del drago per 142 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 29%.");			
		}			
	}
}

	if (item->GetVnum() == PERFECT_CRITICAL_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");	
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_CRITICAL_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		if (IsAffectFlag(AFF_GICHEON))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Aiuto del Dio Drago.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}	
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_CRITICAL_BUFF, POINT_CRITICAL_PCT, PERFECT_CRITICAL_BLESS_AFFECT_VALUE, AFF_CRITICAL_BUFF, PERFECT_CRITICAL_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= PERFECT_CRITICAL_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Aiuto del drago per 185 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 35%.");			
		}			
	}
}

	if (item->GetVnum() == MASTER_REFLECT_BLESS_ITEM_BUFF_VNUM)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_REFLECT_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}
		if (IsAffectFlag(AFF_BOHO))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}		
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_REFLECT_BUFF, POINT_REFLECT_MELEE, MASTER_REFLECT_BLESS_AFFECT_VALUE, AFF_REFLECT_BUFF, MASTER_REFLECT_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= MASTER_REFLECT_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Riflessione per 160 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 21%.");			
		}			
	}
}

	if (item->GetVnum() == GRAND_REFLECT_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_REFLECT_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}
		if (IsAffectFlag(AFF_BOHO))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_REFLECT_BUFF, POINT_REFLECT_MELEE, GRAND_REFLECT_BLESS_AFFECT_VALUE, AFF_REFLECT_BUFF, GRAND_REFLECT_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= GRAND_REFLECT_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Riflessione per 224 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 31%.");			
		}			
	}
}

	if (item->GetVnum() == PERFECT_REFLECT_BLESS_ITEM_BUFF_VNUM) // 
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre stai commerciando o hai altre finestre aperte.");
			return false;
		}
		if (true == IsDead())
		{
			ChatPacket(CHAT_TYPE_INFO, "Non puoi usare i buff mentre sei morto.");
			return false;
		}	
		else
		{
		if (FindAffect(SKILL_REFLECT_BUFF))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}
		if (IsAffectFlag(AFF_BOHO))
		{	
			ChatPacket(CHAT_TYPE_INFO, "Sei gia' benedetto con: Riflessione.");	
			ChatPacket(CHAT_TYPE_INFO, "Attendi la fine del tuo buff e riprova.");				
			return false;
		}
		{
			item->SetSocket(0, item->GetSocket(0) + 1);			
			AddAffect(SKILL_REFLECT_BUFF, POINT_REFLECT_MELEE, PERFECT_REFLECT_BLESS_AFFECT_VALUE, AFF_REFLECT_BUFF, PERFECT_REFLECT_BLESS_AFFECT_TIME_VALUE, 0, true, true);
			if (item->GetSocket(0) >= PERFECT_REFLECT_BLESS_USINGS)
			{
			ITEM_MANAGER::instance().RemoveItem(item);
			}	
			ChatPacket(CHAT_TYPE_INFO, "Sei stato benedetto con: Riflessione per 310 secondi.");
			ChatPacket(CHAT_TYPE_INFO, "Buff ricevuto: 45%.");			
		}			
	}
}
#endif	
	
	//END_PREVENT_TRADE_WINDOW

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi usare questo item se hai una quest aperta."));
		return false;
	}
	// @fixme150 END

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG))
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 16 + 1];
		int len = snprintf(hint, sizeof(hint) - 16, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 16)
			len = (sizeof(hint) - 16) - 1;

		bool ret = UseItemEx(item, DestCell);


		if (NULL == ITEM_MANAGER::instance().FindByVID(vid))
		{
			LogManager::instance().ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::instance().ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

bool CHARACTER::DropItem(TItemPos Cell, BYTE bCount)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ȭâ�� �� ���¿����� �������� �ű� �� �����ϴ�."));
		return false;
	}
#ifdef ENABLE_NEWSTUFF
	if (0 != g_ItemDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastItemDropTime+g_ItemDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��带 ���� �� �����ϴ�."));
			return false;
		}
	}

	m_dwLastItemDropTime = get_dword_time();
#endif
	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;
	
#ifdef ENABLE_RENEWAL_PVP
	if (CheckPvPUse(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1005"));
		return false;
	}
#endif

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �� ���� �������Դϴ�."));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);	// Quickslot ���� ����

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
		LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �������� 3�� �� ������ϴ�."));
#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
#else
		pkItemToDrop->StartDestroyEvent();
#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);

		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

#ifdef ENABLE_SELL_ITEM
bool CHARACTER::SellItem(TItemPos Cell)
{	
	LPITEM item = NULL;
	
	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("#4 <Unknown translate> Report GM"));
		return false;
	}

	if (IsDead())
		return false;
	
	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;
	
	if (item->IsExchanging() || item->IsEquipped())
		return false;
	
	if (true == item->isLocked())
		return false;
	
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return false;

    if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
        return false;
	
    if (item->GetCount() <= 0)
        return false;
	
	long long dwPrice = item->GetShopBuyPrice();
	dwPrice *= item->GetCount();
	
	const long long nTotalMoney = static_cast<long long>(GetGold()) + static_cast<long long>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] OriGold %lld AddedGold %lld id %u Name %s ", GetGold(), dwPrice, GetPlayerID(), GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have reached the maximum limit of yang."));
		return false;
	}
	
	item->SetCount(item->GetCount() - item->GetCount());
	PointChange(POINT_GOLD, dwPrice, false);
	
	char buf[1024];
	char itemlink[256];
	int len;
	len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x", item->GetVnum(), item->GetFlag());

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x", item->GetSocket(i));
		
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		if (item->GetAttributeType(i) != 0)
			len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%d", item->GetAttributeType(i), item->GetAttributeValue(i));
	
	snprintf(buf, sizeof(buf), "|cffffc700|H%s|h[%s]|h|r", itemlink, item->GetName());
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You succesfully sell %s."), buf);
    return true;
}
#endif


bool CHARACTER::DropGold(int gold)
{
	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

	if (0 != g_GoldDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastGoldDropTime+g_GoldDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ��带 ���� �� �����ϴ�."));
			return false;
		}
	}

	m_dwLastGoldDropTime = get_dword_time();

	LPITEM item = ITEM_MANAGER::instance().CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			//Motion(MOTION_PICKUP);
			PointChange(POINT_GOLD, -gold, true);

			if (gold > 1000) // õ�� �̻� ����Ѵ�.
				LogManager::instance().CharLog(this, gold, "DROP_GOLD", "");

#ifdef ENABLE_NEWSTUFF
			item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPGOLD]);
#else
			item->StartDestroyEvent();
#endif
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �������� %d�� �� ������ϴ�."), 150/60);
		}

		Save();
		return true;
	}

	return false;
}

#ifdef FAST_EQUIP_WORLDARD
bool CHARACTER::ChechPositionAvailable(int iWearCell)
{
	if(iWearCell < 0){
		return false;
	}

	for (int a = 0; a <sizeof(EWearCheckPositions)/sizeof(*EWearCheckPositions); ++a)
	{
		if(iWearCell == EWearCheckPositions[a]){
			return true;
		}
	}

	return false;

}

int CHARACTER::IsWearUniqueChangeEquip(BYTE page_index_ce,LPITEM item)
{
	DWORD index_old = CHANGE_EQUIP_SLOT_COUNT-(CHANGE_EQUIP_SLOT_COUNT/page_index_ce);

	if(page_index_ce > 1){
		index_old = CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*page_index_ce;
	}


	LPITEM item_check = GetChangeEquipItem(index_old+WEAR_UNIQUE1);
	if(item_check && item)
	{
		if(item->GetVnum() == item_check->GetVnum())
		{
			return WEAR_UNIQUE1;
		}

		return WEAR_UNIQUE2;
	}

	return WEAR_UNIQUE1;
}


bool CHARACTER::IsValidItemChangeEquip(int cell, LPITEM item)
{	

	if(cell < 0)
	{
		return false;
	}


	if(cell > CHANGE_EQUIP_SLOT_COUNT)
	{
		return false;
	}


	BYTE page_index_ce = 1;

	for (int i = 1; i < CHANGE_EQUIP_PAGE_EXTRA; ++i)
	{
		if(cell >= CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*i && cell < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(i+1))
		{
			page_index_ce = i;
		}
	}

	int iWearCell = item->FindEquipCell(this);

	if (item->GetType() == ITEM_UNIQUE)
	{
		iWearCell = IsWearUniqueChangeEquip(page_index_ce,item);
	}

	if(!ChechPositionAvailable(iWearCell)){
		return false;
	}

	if(GetChangeEquipItem(cell)){
		return false;
	}

	if (!item->CheckItemUseLevel(GetLevel())){
		return false;
	}	

	if(iWearCell == WEAR_ARROW){
		return false;
	}


	for (int i = 1; i < CHANGE_EQUIP_PAGE_EXTRA; ++i)
	{
		if(cell >= CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*i && cell < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(i+1))
		{
			cell = cell - ((CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*i);
		}
	}

	if(iWearCell != cell)
	{
		return false;
	}	

	if ((item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON) || item->GetType() == ITEM_WEAPON)
	{
		
		DWORD index_old = CHANGE_EQUIP_SLOT_COUNT-(CHANGE_EQUIP_SLOT_COUNT/page_index_ce);

		if(page_index_ce > 1){
			index_old = CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*page_index_ce;
		}

		LPITEM check_weapon = NULL;
		LPITEM check_costume = NULL;

		for (int i = index_old; i < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(page_index_ce+1); ++i)
		{
			LPITEM item_check = GetChangeEquipItem(i);
			if(item_check)
			{
				if(item_check->GetType() == ITEM_WEAPON)
				{
					check_weapon = item_check;
				}

				if(item_check->GetType() == ITEM_COSTUME && item_check->GetSubType() == COSTUME_WEAPON)
				{
					check_costume = item_check;
				}
			}
		}

		if(item->GetType() == ITEM_WEAPON)
		{
			if(check_costume != NULL)
			{
				if(check_costume->GetValue(3) != item->GetSubType()){
					return false;
				}
			}
		}

		if(item->GetSubType() == COSTUME_WEAPON)
		{
			if(check_weapon != NULL)
			{
				if(item->GetValue(3) != check_weapon->GetSubType()){
					return false;
				}
			}
		}

	}

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
	}

	if (false == FN_check_item_sex(this, item))
	{
		return false;
	}
	
	return true;
}

#endif

bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, BYTE count)
{
#ifdef __SPECIAL_STORAGE_SYSTEM__
	if (DestCell.window_type == INVENTORY && (Cell.window_type >= SKILLBOOK_INVENTORY && Cell.window_type <= GENERAL_INVENTORY))
	{
		if (DestCell.cell > 4)
		{
			DWORD itemSlot = DestCell.cell - 5;

			LPITEM tmpItem = GetInventoryItem(itemSlot);

			if (DestCell.cell >= 10)
			{
				itemSlot = DestCell.cell - 10;

				if (GetInventoryItem(itemSlot) && GetInventoryItem(itemSlot)->GetSize() == 3)
					return false;
			}

			if (tmpItem)
			{
				if (tmpItem->GetSize() >= 2)
					return false;
			}
		}
	}
#endif

	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ȭâ�� �� ���¿����� �������� �ű� �� �����ϴ�."));
		return false;
	}

	// ��ȹ���� ��û���� ��Ʈ �κ��丮���� Ư�� Ÿ���� �����۸� ���� �� �ִ�.
#ifdef FAST_EQUIP_WORLDARD
	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item) && !DestCell.IsChangeEquipPosition())
#else
	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
#endif
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ��Ʈ �κ��丮�� �ű� �� �����ϴ�."));
		return false;
	}

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		if (CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
		{
			ChatPacket(CHAT_TYPE_INFO, "Cannot move active switchbot item.");
			return false;
		}
		
		if (DestCell.IsEquipPosition())
		{
			ChatPacket(CHAT_TYPE_INFO, "Transfer type not allowed.");
			return false;
		}
	}
	
	if (DestCell.IsSwitchbotPosition())
	{
		if (item->IsEquipped())
		{
			ChatPacket(CHAT_TYPE_INFO, "Transfer type not allowed.");
			return false;
		}
		
		if (!SwitchbotHelper::IsValidItem(item))
		{
			ChatPacket(CHAT_TYPE_INFO, "Invalid item type for switchbot.");
			return false;
		}
	}
#endif

#ifdef FAST_EQUIP_WORLDARD
	if(DestCell.IsChangeEquipPosition())
	{
		if(!IsValidItemChangeEquip(DestCell.cell,item)){
			ChatPacket(CHAT_TYPE_INFO,"No puedes mover el item a ese slot");
			return false;
		}

		if(Cell.IsEquipPosition()){
			return false;
		}
	}
#endif

	// �̹� �������� �������� �ٸ� ������ �ű�� ���, '��å ����' ������ �� Ȯ���ϰ� �ű�
#ifdef FAST_EQUIP_WORLDARD
	if (Cell.IsEquipPosition() && !Cell.IsChangeEquipPosition())
#else
	if (Cell.IsEquipPosition())
#endif
	{
		if (!CanUnequipNow(item))
			return false;

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		int iWearCell = item->FindEquipCell(this);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"You cannot unequip the costume weapon. Not enough space."));
				return false;
			}

			if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
				return UnequipItem(item);
		}
#endif
	}

#ifdef FAST_EQUIP_WORLDARD
	if (DestCell.IsEquipPosition() && !DestCell.IsChangeEquipPosition())
#else
	if (DestCell.IsEquipPosition())
#endif	
	{
		if (GetItem(DestCell))	// ����� ��� �� ���� �˻��ص� �ȴ�.
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� ��� �����ϰ� �ֽ��ϴ�."));

			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}
		// ��ȥ���� �ƴ� �������� ��ȥ�� �κ��� �� �� ����.
		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;
		
#ifdef __SPECIAL_STORAGE_SYSTEM__
		switch (DestCell.window_type)
		{
			case SKILLBOOK_INVENTORY:
			case UPPITEM_INVENTORY:
			case GHOSTSTONE_INVENTORY:
			case GENERAL_INVENTORY:
			{
				char szTemp[256];
				snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), item->GetCount());

				if (Cell.window_type != DestCell.window_type && Cell.window_type != INVENTORY)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"The item %s%s is not allowed to be stored here."), item->GetName(GetLanguage()), (item->GetCount() > 1 ? szTemp : " "));
					return false;
				}
				else if (Cell.window_type == INVENTORY)
				{
					if (item->GetSpecialWindowType() != DestCell.window_type)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"The item %s%s is not allowed to be stored here."), item->GetName(GetLanguage()), (item->GetCount() > 1 ? szTemp : " "));
						return false;
					}
				}
				else if (Cell.window_type >= SKILLBOOK_INVENTORY && Cell.window_type <= GENERAL_INVENTORY)
				{
					if (GetItem(DestCell) && GetItem(Cell) && GetItem(DestCell)->GetVnum() == GetItem(Cell)->GetVnum())
					{
						if (DestCell.cell == Cell.cell)
							return false;
					}
				}
			}
			break;
		}
#endif

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // ��ĥ �� �ִ� �������� ���
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(GetLanguage()), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
#ifndef __SWAPITEM_SYSTEM__
			return false;
#else
		{
			if (Cell.IsEquipPosition() || DestCell.IsEquipPosition())
				return false;
			
			if (item->GetType() == ITEM_ARMOR or item->GetType() == ITEM_WEAPON or item->GetType() == ITEM_COSTUME)
			{
				LPITEM itemSrc = GetInventoryItem(Cell.cell);
				LPITEM itemDest = GetInventoryItem(DestCell.cell);
				if (itemSrc == NULL || itemDest == NULL)
					return false;
				
				int itemSrcSize = itemSrc->GetSize();
				int itemDestSize = itemDest->GetSize();
				if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || GetOfflineShopOwner() || GetMailBox())
					return false;
#ifdef __COSTUME_ATTR_TRANSFER__
				if (IsAttrTransferOpen())
					return false;
#endif
				
#ifdef __OFFLINE_SHOP_SYSTEM__
				if (GetOfflineShop())
					return false;
#endif
				
#ifdef __ATTR_TRANSFER_SYSTEM__
				if (IsAttrTransferOpen())
					return false;
#endif
				
				if (itemSrc->isLocked() || itemDest->isLocked())
					return false;
				else if (itemDestSize > itemSrcSize)
					return false;
				else if (itemSrcSize == itemDestSize)
				{
					itemSrc->RemoveFromCharacter();
					itemDest->RemoveFromCharacter();
					SetItem(Cell, itemDest);
					SetItem(DestCell, itemSrc);
				}
				else if (itemSrcSize > itemDestSize)
				{
#ifdef __4_INVENTORY_PAGES__
					if (itemDestSize == 1 && itemDest->GetCell() >= 40 && itemDest->GetCell() < 45 || itemDestSize == 1 && itemDest->GetCell() >= 85 && itemDest->GetCell() < 90 || itemDestSize == 1 && itemDest->GetCell() >= 130 && itemDest->GetCell() < 135 || itemDestSize == 1 && itemDest->GetCell() >= 175 && itemDest->GetCell() < 180)
#else
					if (itemDestSize == 1 && itemDest->GetCell() >= 40 && itemDest->GetCell() < 45 || itemDestSize == 1 && itemDest->GetCell() >= 85 && itemDest->GetCell() < 90)
#endif
						return false;
					
#ifdef __4_INVENTORY_PAGES__
					if (itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 35 && itemDest->GetCell() < 40 || itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 80 && itemDest->GetCell() < 85 || itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 125 && itemDest->GetCell() < 130 || itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 170 && itemDest->GetCell() < 175)
#else
					if (itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 35 && itemDest->GetCell() < 40 || itemDestSize == 1 && itemSrcSize == 3 && itemDest->GetCell() >= 80 && itemDest->GetCell() < 85)
#endif
						return false;
					
#ifdef __4_INVENTORY_PAGES__
					if (itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 35 && itemDest->GetCell() < 40 || itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 80 && itemDest->GetCell() < 85 || itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 125 && itemDest->GetCell() < 130 || itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 170 && itemDest->GetCell() < 175)
#else
					if (itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 35 && itemDest->GetCell() < 40 || itemDestSize == 2 && itemSrcSize == 3 && itemDest->GetCell() >= 80 && itemDest->GetCell() < 85)
#endif
						return false;
					
					bool move_ = SwapItem(item->GetCell(), item2->GetCell());
					if (!move_)
						return false;
					
					BYTE bCell = Cell.cell;
					BYTE bDestCell = DestCell.cell;
					if (itemSrcSize == 2 && itemDestSize == 1)
					{
						bCell = bCell + 5;
						bDestCell = bDestCell + 5;
						LPITEM itemCheck = GetInventoryItem(bDestCell);
						if (itemCheck != NULL)
						{
							if (itemCheck->GetSize() != 1 || itemCheck->isLocked())
							{
								SwapItem(item2->GetCell(), item->GetCell());
								return false;
							}
							
							itemCheck->RemoveFromCharacter();
							SetItem(TItemPos(INVENTORY, bCell), itemCheck);
						}
					}
					else if (itemSrcSize == 3)
					{
						if (itemDestSize == 2)
						{
							bCell = bCell + 10;
							bDestCell = bDestCell + 10;
							LPITEM itemCheck = GetInventoryItem(bDestCell);
							if (itemCheck != NULL)
							{
								if (itemCheck->GetSize() != 1 || itemCheck->isLocked())
								{
									SwapItem(item2->GetCell(), item->GetCell());
									return false;
								}
								
								itemCheck->RemoveFromCharacter();
								SetItem(TItemPos(INVENTORY, bCell), itemCheck);
							}
						}
						else
						{
							bCell = bCell + 5;
							bDestCell = bDestCell + 5;
							LPITEM itemCheck = GetInventoryItem(bDestCell);
							if (itemCheck == NULL)
							{
								bCell = bCell + 5;
								bDestCell = bDestCell + 5;
								LPITEM itemCheckTwo = GetInventoryItem(bDestCell);
								if (itemCheckTwo != NULL)
								{
									if (itemCheckTwo->GetSize() != 1 || itemCheckTwo->isLocked())
									{
										SwapItem(item2->GetCell(), item->GetCell());
										return false;
									}
									
									itemCheckTwo->RemoveFromCharacter();
									SetItem(TItemPos(INVENTORY, bCell), itemCheckTwo);
								}
							}
							else
							{
								if (itemCheck->GetSize() == 3 || itemCheck->isLocked())
								{
									SwapItem(item2->GetCell(), item->GetCell());
									return false;
								}
								
								if (itemCheck->GetSize() == 1)
								{
									bCell = bCell + 5;
									bDestCell = bDestCell + 5;
									BYTE bCellOld = bCell - 5;
									LPITEM itemCheckThree = GetInventoryItem(bDestCell);
									if (itemCheckThree != NULL)
									{
										if (itemCheckThree->GetSize() != 1 || itemCheckThree->isLocked())
										{
											SwapItem(item2->GetCell(), item->GetCell());
											return false;
										}
										
										itemCheck->RemoveFromCharacter();
										SetItem(TItemPos(INVENTORY, bCellOld), itemCheck);
										itemCheckThree->RemoveFromCharacter();
										SetItem(TItemPos(INVENTORY, bCell), itemCheckThree);
									}
									else
									{
										itemCheck->RemoveFromCharacter();
										SetItem(TItemPos(INVENTORY, bCellOld), itemCheck);
									}
								}
								else
								{
									itemCheck->RemoveFromCharacter();
									SetItem(TItemPos(INVENTORY, bCell), itemCheck);
								}
							}
						}
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
#endif

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(GetLanguage()), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->RemoveFromCharacter();
			SetItem(DestCell, item);

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{

			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(GetLanguage()), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item)
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center )
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);

					if (iMoney > 1000) // õ�� �̻� ����Ѵ�.
					{
						LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(ch, iMoney, "GET_GOLD", ""));
					}
				}
		}
	};
}

void CHARACTER::GiveGold(int iAmount)
{
	if (iAmount <= 0)
		return;

	sys_log(0, "GIVE_GOLD: %s %d", GetName(), iAmount);


	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		// ��Ƽ�� �ִ� ��� ������ ������.
		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

		if (dwMyAmount > 1000) // õ�� �̻� ����Ѵ�.
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, dwMyAmount, "GET_GOLD", ""));
		}
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);

		if (iAmount > 1000) // õ�� �̻� ����Ѵ�.
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, iAmount, "GET_GOLD", ""));
		}
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"You cannot pickup this item if you're using quests"));
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			// ���� ������ �ϴ� �������� ��ũ���
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();
				M2_DESTROY_ITEM(item);

				Save();
			}
			// ����� �������̶��
			else
			{
#ifdef __SPECIAL_STORAGE_SYSTEM__
				if (item->IsSpecialStorageItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					BYTE bCount = item->GetCount();

					const char * GetStorageName[4] = {	LC_TEXT("Carti - Inventar"),
														LC_TEXT("Upgrade - Inventar"),
														LC_TEXT("Pietre - Inventar"),
														LC_TEXT("General - Inventar")
					};

					int GetSpecialWindowType = item->GetSpecialWindowType() - 6;

					for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetSpecialStorageItem(i, item->GetSpecialWindowType());

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

							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{

								char szTemp[256];
								snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), item->GetCount());
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai primit: %s"), item2->GetName());	
								M2_DESTROY_ITEM(item);

								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
				else if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#else
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#endif
				{
					BYTE bCount = item->GetCount();

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

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

							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
#if defined(__CHATTING_WINDOW_RENEWAL__)
ChatPacket(CHAT_TYPE_ITEM_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s"), item->GetName());
#else
ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai ricevuto: %s"), item->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);

				}

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
						return false;
					}
				}
#ifdef __SPECIAL_STORAGE_SYSTEM__
				else if (item->IsSpecialStorageItem())
				{
					const char* GetStorageName[4] = {	LC_TEXT("Carti - Inventar"),
														LC_TEXT("Upgrade - Inventar"),
														LC_TEXT("Pietre - Inventar"),
														LC_TEXT("General - Inventar")
					};

					int GetSpecialWindowType = item->GetSpecialWindowType() - 6;

					iEmptyCell = GetEmptySpecialStorageSlot(item);

					if (iEmptyCell == -1)
					{
						iEmptyCell = GetEmptyInventory(item->GetSize());

						if (iEmptyCell == -1)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
							return false;
						}

						char szTemp[256];
						snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), item->GetCount());

						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"%s has no space, so the item %s%s moving automatically to inventory."), GetStorageName[GetSpecialWindowType], item->GetName(GetLanguage()), (item->GetCount() > 1 ? szTemp : " "));

						item->RemoveFromGround();
						item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

						return true;
					}
					else if (iEmptyCell != -1)
					{
						char szTemp[256];
						snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), item->GetCount());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto %s%s - puoi trovarlo nell'inventario speciale. [PREMI U]"), item->GetName(GetLanguage()), (item->GetCount() > 1 ? szTemp : " "), GetStorageName[GetSpecialWindowType]);

						item->RemoveFromGround();
						item->AddToCharacter(this, TItemPos(item->GetSpecialWindowType(), iEmptyCell));

						return true;
					}
				}

				if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
				{
					sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
					return false;
				}
#else
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
						return false;
					}
				}
#endif

				item->RemoveFromGround();

				if (item->IsDragonSoul()){
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
					//ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item is gay."));
				}
#ifdef __SPECIAL_STORAGE_SYSTEM__
				else if (item->IsSpecialStorageItem())
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
#endif
				else
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(GetLanguage()), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "GET", szHint);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai primit: %s"), item->GetName());	
				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			// �ٸ� ��Ƽ�� ������ �������� �������� �Ѵٸ�
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			// @fixme115
			if (!owner)
				return false;

			int iEmptyCell;
			
#ifdef __SPECIAL_STORAGE_SYSTEM__
			if (item->IsSpecialStorageItem() && owner && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
			{
				BYTE bCount = item->GetCount();

				for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = owner->GetSpecialStorageItem(i, item->GetSpecialWindowType());

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

						BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;

						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
						{
							if (item->GetCount() == 1)
							{
								owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), owner->GetName(), item->GetName(GetLanguage()));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), owner->GetName(), item->GetName(GetLanguage()));
							}
							else
							{
								owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), owner->GetName(), item->GetName(GetLanguage()), item->GetCount());
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), owner->GetName(), item->GetName(GetLanguage()), item->GetCount());
							}

							M2_DESTROY_ITEM(item);

							return true;
						}
					}
				}

				item->SetCount(bCount);
			}
			else if (owner && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#else
			if (owner && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#endif
			{
				BYTE bCount = item->GetCount();

				for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				{
					LPITEM item2 = owner->GetInventoryItem(i);

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

						BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
						bCount -= bCount2;


						item2->SetCount(item2->GetCount() + bCount2);

						if (bCount == 0)
						{
							if (item->GetCount() == 1)
							{
								owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), owner->GetName(), item->GetName(GetLanguage()));
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Consegna l'item: %s , %s"), owner->GetName(), item->GetName(GetLanguage()));
							}
							else
							{
								owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s x%d"), owner->GetName(), item->GetName(GetLanguage()), item->GetCount());
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Consegna l'item: %s , %s x%d"), owner->GetName(), item->GetName(GetLanguage()), item->GetCount());
							}

							M2_DESTROY_ITEM(item);

							if (item2->GetType() == ITEM_QUEST)
								quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item2);

							return true;
						}
					}
				}

				item->SetCount(bCount);
			}

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
						return false;
					}
				}
			}
#ifdef __SPECIAL_STORAGE_SYSTEM__
			else if (item->IsSpecialStorageItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptySpecialStorageSlot(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptySpecialStorageSlot(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
						return false;
					}
				}
			}
#endif
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�����ϰ� �ִ� �������� �ʹ� �����ϴ�."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#ifdef __SPECIAL_STORAGE_SYSTEM__
			else if (item->IsSpecialStorageItem())
				item->AddToCharacter(owner, TItemPos(item->GetSpecialWindowType(), iEmptyCell));
#endif
			else
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(GetLanguage()), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai primit: %s"), item->GetName());	
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai ricevuto: %s , %s"), GetName(), item->GetName(GetLanguage()));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Consegna l'item: %s , %s"), owner->GetName(), item->GetName(GetLanguage()));
			}

			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem (owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(BYTE bCell, BYTE bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	// �ùٸ� Cell ���� �˻�
	// ��ȥ���� Swap�� �� �����Ƿ�, ���⼭ �ɸ�.
	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	// ���� CELL ���� �˻�
	if (bCell == bDestCell)
		return false;

	// �� �� ���â ��ġ�� Swap �� �� ����.
	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	// item2�� ���â�� �ִ� ���� �ǵ���.
	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;

	if (item1 == item2)
	{
	    sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), bCell, bDestCell);
	    return false;
	}

	// item2�� bCell��ġ�� �� �� �ִ��� Ȯ���Ѵ�.
	if (!IsEmptyItemGrid(TItemPos (INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	// �ٲ� �������� ���â�� ������
	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		// �������� �������� ���� �� �ְ�, ���� ���� �������� ���� ������ ���¿��߸� ����
		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this)) // ���� ��ġ�϶��� ���
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
#ifdef __HIGHLIGHT_SYSTEM__
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell), false);
#else
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
#endif
		else
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
	}
	else
	{
		BYTE bCell1 = item1->GetCell();
		BYTE bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

#ifdef __HIGHLIGHT_SYSTEM__
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2), false);
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1), false);
#else
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
#endif
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"You cannot unequip the costume weapon. Not enough space."));
			return false;
		}
	}
#endif

	if (false == CanUnequipNow(item))
		return false;

	int pos;
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
#ifdef __HIGHLIGHT_SYSTEM__
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
#endif
	}
	else
#ifdef __HIGHLIGHT_SYSTEM__
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

	CheckMaximumPoints();

	return true;
}

//
// @version	05/07/05 Bang2ni - Skill ����� 1.5 �� �̳��� ��� ���� ����
//
bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{

	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;

	// ���𰡸� ź ���¿��� �νõ� �Ա� ����
	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ź ���¿��� ������ ���� �� �����ϴ�."));
		return false;
	}

	/*if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�а� �߿��� �������� ��� ������ �� �����ϴ�."));
		return false;
	}*/

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �����ʾ� �� �������� ����� �� �����ϴ�."));
		return false;
	}

	//�ű� Ż�� ���� ���� �� ��뿩�� üũ
	if(item->IsRideItem() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Hai gia' evocato una cavalcatura."));
		return false;
	}

	// ȭ�� �̿ܿ��� ������ ���� �ð� �Ǵ� ��ų ��� 1.5 �Ŀ� ��� ��ü�� ����
	DWORD dwCurTime = get_dword_time();

	//if (iWearCell != WEAR_ARROW	&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
		
	/* mount disable check "still to use item" 05/11/2017 */
	if(!item->IsMountItemCostume() && iWearCell != WEAR_ARROW && (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500) )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���� ���� ������ �� �ֽ��ϴ�."));
		return false;
	}

#if defined(ENABLE_MOUNT_COSTUME_SYSTEM) && defined(ENABLE_DECORUM)
	if(item->IsMountItemCostume() && CDecoredArenaManager::instance().IsArenaMap(this->GetMapIndex()) == true)
		return false;
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"You cannot unequip the costume weapon. Not enough space."));
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi togliere la skin arma, non hai sufficiente spazio."));
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Non puoi usare questa skin su quest'arma."));
				return false;
			}
		}
	}
#endif

	// ��ȥ�� Ư�� ó��
	if (item->IsDragonSoul())
	{
		// ���� Ÿ���� ��ȥ���� �̹� �� �ִٸ� ������ �� ����.
		// ��ȥ���� swap�� �����ϸ� �ȵ�.
		if(GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "�̹� ���� ������ ��ȥ���� �����ϰ� �ֽ��ϴ�.");
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	// ��ȥ���� �ƴ�.
	else
	{
		// ������ ���� �������� �ִٸ�,
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			// �� �������� �ѹ� ������ ���� �Ұ�. swap ���� ���� �Ұ�
			if (item->GetWearFlag() == WEARABLE_ABILITY)
				return false;

			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (true == item->IsEquipped())
	{
		// ������ ���� ��� ���ĺ��ʹ� ������� �ʾƵ� �ð��� �����Ǵ� ��� ó��.
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			// �� ���̶� ����� ���������� ���δ� Socket1�� ���� �Ǵ��Ѵ�. (Socket1�� ���Ƚ�� ���)
			if (0 == item->GetSocket(1))
			{
				// ��밡�ɽð��� Default ������ Limit Value ���� ����ϵ�, Socket0�� ���� ������ �� ���� ����ϵ��� �Ѵ�. (������ ��)
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		// �󸶴� �̺�Ʈ �ʽ´��� ����(71135) ����� ����Ʈ �ߵ�
		if (true == CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}
		// �ҷ��� ����(71136) ����� ����Ʈ �ߵ�
		else if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
#ifdef ENABLE_SYSTEM_RUNE
		// WHITE
		else if (true == CItemVnumHelper::IsRunaWhite(dwVnum))
		{
			this->EffectPacket(SE_RUNA_WHITE_EFFECT);
		}
		// RED
		else if (true == CItemVnumHelper::IsRunaRed(dwVnum))
		{
			this->EffectPacket(SE_RUNA_RED_EFFECT);
		}
		// BLUE
		else if (true == CItemVnumHelper::IsRunaBlue(dwVnum))
		{
			this->EffectPacket(SE_RUNA_BLUE_EFFECT);
		}
		// GREEN
		else if (true == CItemVnumHelper::IsRunaGreen(dwVnum))
		{
			this->EffectPacket(SE_RUNA_GREEN_EFFECT);
		}
		// BLACK
		else if (true == CItemVnumHelper::IsRunaBlack(dwVnum))
		{
			this->EffectPacket(SE_RUNA_BLACK_EFFECT);
		}
		// YELLOW
		else if (true == CItemVnumHelper::IsRunaYellow(dwVnum))
		{
			this->EffectPacket(SE_RUNA_YELLOW_EFFECT);
		}
#endif
		// �ູ�� ����(71143) ����� ����Ʈ �ߵ�
		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		// ����� �Ҵ�Ʈ(71145) ����� ����Ʈ �ߵ�
		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		// ITEM_UNIQUE�� ���, SpecialItemGroup�� ���ǵǾ� �ְ�, (item->GetSIGVnum() != NULL)
		//
#ifdef ENABLE_AURA_SYSTEM
		else if(item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_AURA)
		{
			SpecificEffectPacket("d:/ymir work/effect/etc/buff/buff_wing7.mse");
		}
#endif
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if ((item->GetType() == ITEM_COSTUME) && (item->GetSubType() == COSTUME_ACCE))
			this->EffectPacket(SE_EFFECT_ACCE_EQUIP);
#endif

#ifdef ENABLE_TALISMAN_EFFECT
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 9600 && item->GetVnum()<= 9800))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_FIRE);
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 9830 && item->GetVnum()<= 10030))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_ICE);
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 10520 && item->GetVnum()<= 10720))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_EARTH);
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 10060 && item->GetVnum()<= 10260))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_WIND);
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 10290 && item->GetVnum()<= 10490))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_DARK);
	else if (/*(item->GetType() == ITEM_ARMOR) && (item->GetWearFlag() ==WEARABLE_PENDANT) && */(item->GetVnum() >= 10750 && item->GetVnum()<= 10950))
		this->EffectPacket(SE_EFFECT_TALISMAN_EQUIP_ELEC);
#endif

		if (
			(ITEM_UNIQUE == item->GetType() && UNIQUE_SPECIAL_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))
			|| (ITEM_UNIQUE == item->GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			|| (ITEM_COSTUME == item->GetType() && COSTUME_MOUNT == item->GetSubType())
#endif
			)
		{
			quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
		}

	}

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = {
						WEAR_COSTUME_BODY,
						WEAR_COSTUME_HAIR,
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
						WEAR_COSTUME_MOUNT,
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
						WEAR_COSTUME_WEAPON,
#endif
#ifdef ENABLE_EFFECT_COSTUME_SYSTEM
						WEAR_COSTUME_EFFECT,
#endif
					};
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));

		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert (m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}


LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (int i = 0; i < malibuclub; ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)	
#endif
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			LPITEM item = GetSpecialStorageItem(i, j);
			if (item && item->GetVnum() == vnum)
				return item;
		}
	}
#endif

	return NULL;
}

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (int i = 0; i < malibuclub; ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)	
#endif
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}
	
#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			LPITEM item = GetSpecialStorageItem(i, j);
			if (item && item->GetID() == id)
				return item;
		}
	}
#endif

	return NULL;
}

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	int	count = 0;
	LPITEM item;
	//this will end in a lot of work for me -.-
#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (int i = 0; i < malibuclub; ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)	
#endif
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			// �Ʃ�AI ��oA������ ��i��I��E ���ơ�CAI��e ��N��i�ơ̢�U.
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}
	
#ifdef __SPECIAL_STORAGE_SYSTEM__
	//This will also count items in all your special storages
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			item = GetSpecialStorageItem(i, j);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
				{
					continue;
				}
				else
				{
					count += item->GetCount();
				}
			}
		}
	}
#endif

	return count;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (0 == count)
		return;

#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (UINT i = 0; i < malibuclub; ++i)
#else
	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		//�Ʃ�AI ��oA������ ��i��I��E ���ơ�CAI��e ��N��i�ơ̢�U. (�Ʃ�AI ��oA���������� ��C��A��E�ҡ� AI ��I����A����I ��e��i��A �Ʃ���i ����A|!)
		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (vnum >= 80003 && vnum <= 80007)
			LogManager::instance().GoldBarLog(GetPlayerID(), GetInventoryItem(i)->GetID(), QUEST, "RemoveSpecifyItem");

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
	
#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			LPITEM item = GetSpecialStorageItem(i, j);
			if (NULL == item)
				continue;

			if (item->GetVnum() != vnum)	
				continue;
			
			if(m_pkMyShop)
			{
				bool isItemSelling = m_pkMyShop->IsSellingItem(item->GetID());
				if (isItemSelling)
					continue;
			}

			if (vnum >= 80003 && vnum <= 80007)
				LogManager::instance().GoldBarLog(GetPlayerID(), item->GetID(), QUEST, "RemoveSpecifyItem");

			if (count >= item->GetCount())
			{
				count -= item->GetCount();
				item->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				item->SetCount(item->GetCount() - count);
				return;
			}
		}
	}
#endif

	// ������UA������Ƣ� ��aCI��U.
	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;
#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (UINT i = 0; i < malibuclub; ++i)
#else
	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}
	
#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			LPITEM pItem = GetSpecialStorageItem(i, j);
			if (pItem != NULL && pItem->GetType() == type)
			{
				count += pItem->GetCount();
			}
		}
	}
#endif

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

#ifdef NEW_ADD_INVENTORY
	int malibuclub = (90 + (5*Black_Envanter()));
	for (UINT i = 0; i < malibuclub; ++i)
#else
	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		//�Ʃ�AI ��oA������ ��i��I��E ���ơ�CAI��e ��N��i�ơ̢�U. (�Ʃ�AI ��oA���������� ��C��A��E�ҡ� AI ��I����A����I ��e��i��A �Ʃ���i ����A|!)
		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
	
#ifdef __SPECIAL_STORAGE_SYSTEM__
	for (int j = SKILLBOOK_INVENTORY; j <= GENERAL_INVENTORY; ++j) //through all types
	{
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i) //through all slots
		{
			LPITEM item = GetSpecialStorageItem(i, j);
			if (NULL == item)
				continue;

			if (item->GetType() != type)
				continue;

			//�Ʃ�AI ��oA������ ��i��I��E ���ơ�CAI��e ��N��i�ơ̢�U. (�Ʃ�AI ��oA���������� ��C��A��E�ҡ� AI ��I����A����I ��e��i��A �Ʃ���i ����A|!)
			if(m_pkMyShop)
			{
				bool isItemSelling = m_pkMyShop->IsSellingItem(item->GetID());
				if (isItemSelling)
					continue;
			}

			if (count >= item->GetCount())
			{
				count -= item->GetCount();
				item->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				item->SetCount(item->GetCount() - count);
				return;
			}
		}
	}
#endif
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}

	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#ifdef __SPECIAL_STORAGE_SYSTEM__
	else if (item->IsSpecialStorageItem())
		cell = GetEmptySpecialStorageSlot(item);
#endif
	else
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSpecialStorageItem())
/* #ifdef __HIGHLIGHT_SYSTEM__
		{
			if (isHighLight)
				item->AddToCharacter(this, TItemPos(item->GetSpecialWindowType(), cell), false);
			else
				item->AddToCharacter(this, TItemPos(item->GetSpecialWindowType(), cell), true);
		}
#else */
			item->AddToCharacter(this, TItemPos(item->GetSpecialWindowType(), cell));
// #endif
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName(GetLanguage()));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName(GetLanguage()));
	}
}

LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, BYTE bCount, int iRarePct, bool bMsg)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

// #ifdef ENABLE_AUTOGIVEITEM_CELL_CHECK
	// LPITEM itemCheck = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true, -1, true);
	// if (itemCheck)
	// {
		// int iEmptyCell = -1;
		// if (p->bType == ITEM_DS)
			// iEmptyCell = GetEmptyDragonSoulInventory(itemCheck);
// #ifdef __SPECIAL_STORAGE_SYSTEM__
		// else if (itemCheck->IsSpecialStorageItem(dwItemVnum))
			// iEmptyCell = GetEmptySpecialStorageSlot(itemCheck);
// #endif
		// else
			// iEmptyCell = GetEmptyInventory(p->bSize);
		
		// if (iEmptyCell == -1)
		// {
			// if (bMsg)
				// ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Envanterinde yer yok autogiveitem!"));
			// return NULL;
		// }
		
		// M2_DESTROY_ITEM(itemCheck);
    // }
// #endif

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Primit: %s"), item->GetName());	
					return item;
				}
			}
		}
#ifdef __SPECIAL_STORAGE_SYSTEM__
		for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Primit: %s"), item->GetName());	
					return item;
				}
			}
		}
#endif
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
							inv_item->GetSocket(2) == item->GetSocket(2) &&
							inv_item->GetCount() < g_bItemCountLimit)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						M2_DESTROY_ITEM(item);	//@fixme472
						return inv_item;
					}
				}
			}
		}
	}
// #ifdef __SPECIAL_STORAGE_SYSTEM__
	// if (item->IsSpecialStorageItem() && p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	// {
		// LPITEM olditem = item;

		// if (!olditem)
			// return NULL;

		// for (int i = 0; i < SPECIAL_STORAGE_INVENTORY_MAX_NUM; ++i)
		// {
			// LPITEM item = GetSpecialStorageItem(i, olditem->GetSpecialWindowType());

			// if (!item)
				// continue;

			// if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			// {
				// if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				// {
					// if (bCount < p->alValues[1])
						// bCount = p->alValues[1];
				// }

				// BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				// bCount -= bCount2;

				// item->SetCount(item->GetCount() + bCount2);

				// if (bCount == 0)
				// {
					// if (bMsg)
						// ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"Ricevuto: %s"), item->GetName(GetLanguage()));

					// return item;
				// }
			// }
		// }
	// }
// #endif

	int iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
#ifdef __SPECIAL_STORAGE_SYSTEM__
	else if (item->GetSpecialWindowType())
		iEmptyCell = GetEmptySpecialStorageSlot(item);
	else if (item->IsUpgradeItem())
	{
		iEmptyCell = UPPITEM_INVENTORY;
	}
#endif
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ai primit: %s"), item->GetName());	
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#ifdef __SPECIAL_STORAGE_SYSTEM__
		else if (item->IsSpecialStorageItem())
			item->AddToCharacter(this, TItemPos(item->GetSpecialWindowType(), iEmptyCell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName(GetLanguage()));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
		// ��Ƽ ��� flag�� �ɷ��ִ� �������� ���,
		// �κ��� �� ������ ��� ��¿ �� ���� ����Ʈ���� �Ǹ�,
		// ownership�� �������� ����� ������(300��) �����Ѵ�.
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName(GetLanguage()));
	}

	sys_log(0,
		"7: %d %d", dwItemVnum, bCount);
	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"You cannot take this item if you're using quests"));
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH &&
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM_WEAPON &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
#ifdef ENABLE_NEW_TALISMAN_GF
			if (item->GetType() == ITEM_ARMOR &&
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD || item->GetSubType() == ARMOR_TALISMAN
#ifdef ENABLE_NEW_TALISMAN_SLOTS
					 || item->GetSubType() == ARMOR_TALISMAN_2  || item->GetSubType() == ARMOR_TALISMAN_3  || item->GetSubType() == ARMOR_TALISMAN_4
					 || item->GetSubType() == ARMOR_TALISMAN_5  || item->GetSubType() == ARMOR_TALISMAN_6
#endif
					) &&
					item->GetRefinedVnum())
#else
			if (item->GetType() == ITEM_ARMOR &&
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())

#endif
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
#ifdef ENABLE_NEW_TALISMAN_GF
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD || item->GetSubType() == ARMOR_TALISMAN
#ifdef ENABLE_NEW_TALISMAN_SLOTS
					 || item->GetSubType() == ARMOR_TALISMAN_2  || item->GetSubType() == ARMOR_TALISMAN_3  || item->GetSubType() == ARMOR_TALISMAN_4
					 || item->GetSubType() == ARMOR_TALISMAN_5  || item->GetSubType() == ARMOR_TALISMAN_6
#endif
					) &&
					item->GetRefinedVnum())
#else
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())

#endif
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:
			// �ʱ� ��
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���� ������ ���ʸ� ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ��Ḧ ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:
			// �߱� ��
			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���� ������ ���ʸ� ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ��Ḧ ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:
			// ��� ��
			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ���� ������ ���ʸ� ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ��Ḧ ���� �� �����ϴ�."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�� �������� ������ �� �����ϴ�."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���ʸ� �־����ϴ�."));
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ��Ḧ �־����ϴ�."));
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName(GetLanguage()));
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}
#endif

	// �������� ��� ������(�ߺ�) ������ üũ�Ѵ�.
	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum) {
			return true;
		}
	}
#endif

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC( NULL );
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD> &dwItemVnums,
											std::vector<DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, dwCount);
				LogManager::instance().CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::instance().CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + number(-500, 500);
					int y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case CSpecialItemGroup::BLEEDING:
				{
					AttackedByBleeding(NULL);
					bSuccess = true;
				}
				break;
#endif
			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - number(300, 500);
					int sy = GetY() - number(300, 500);
					int ex = GetX() + number(300, 500);
					int ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;
			default:
				{
					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

					if (item_get)
					{
						bSuccess = true;
					}
				}
				break;
		}

		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		// ���� ���ѿ� �ɸ�
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� �� �Ӹ��� ����� �� ���� �����Դϴ�."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000; // 73001 - 72000 = 1001 ���� ��� ��ȣ ����
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			break; // NOTE: �� ����ڵ�� �� ���̹Ƿ� �н�. (���� ���ý����� �̹� �ڽ�Ƭ���� ��ü �� ������)
#endif
		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �Ӹ� ��Ÿ�Ϸδ� ��ü�� �� �����ϴ�."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�̹� �а����� �����Դϴ�."));
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�а��� �� ���� �����Դϴ�."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�߸��� �а� �������Դϴ�."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�߸��� �а� �������Դϴ�."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				// �а��� ó��
				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);

				// ���� ���� üũ
				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ �ʹ� ���� ������ ���ͷδ� ���� �� �� �����ϴ�."));
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = 0;

				dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				// ����

				// �а��� ó��
				// ����0                ����1           ����2
				// �а��� ���� ��ȣ   ��������        �а��� ����
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;
#ifdef OFFLINE_SHOP
	if (GetOfflineShop()) return false;
#endif
	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM Unique3 = GetWear(WEAR_COSTUME_MOUNT);
#endif
	if( NULL != Unique1 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup() )
		{
			return UnequipItem(Unique1);
		}
	}

	if( NULL != Unique2 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup() )
		{
			return UnequipItem(Unique2);
		}
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (NULL != Unique3)
	{
		if (UNIQUE_GROUP_SPECIAL_RIDE == Unique3->GetSpecialGroup())
		{
			return UnequipItem(Unique3);
		}
	}
	// if (MountCostume)
		// return UnequipItem(MountCostume);
#endif

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (!CArenaManager::instance().IsArenaMap(GetMapIndex())
#ifdef ENABLE_NEWSTUFF
				&& !(g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(pItem->GetVnum()))
#endif
			)
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);

				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
#ifndef PERMA_ELIXIRS_S_M
					if (avail > amount)
					{
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;
						// ��뷮�� 10% ������ �α׸� ����
						// (��뷮�� %����, ���� �ڸ��� �ٲ� ������ �α׸� ����.)
						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::instance().RemoveItem( pItem );
					}
#endif
					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange( POINT_HP_RECOVERY, amount );
						EffectPacket( SE_AUTO_HPUP );
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange( POINT_SP_RECOVERY, amount );
						EffectPacket( SE_AUTO_SPUP );
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
#ifdef __SPECIAL_STORAGE_SYSTEM__
		case SKILLBOOK_INVENTORY:
		case UPPITEM_INVENTORY:
		case GHOSTSTONE_INVENTORY:
		case GENERAL_INVENTORY:
			return cell < (SPECIAL_STORAGE_INVENTORY_MAX_NUM);
#endif

	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
#endif
#ifdef FAST_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		return cell < CHANGE_EQUIP_SLOT_COUNT;
#endif
	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;
	default:
		return false;
	}
}


// �����Ƽ� ���� ��ũ��.. exp�� true�� msg�� ����ϰ� return false �ϴ� ��ũ�� (�Ϲ����� verify �뵵���� return ������ �ణ �ݴ�� �̸������� �򰥸� ���� �ְڴ�..)
#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, (msg)); \
			return false; \
	}

/// ���� ĳ������ ���¸� �������� �־��� item�� ������ �� �ִ� �� Ȯ���ϰ�, �Ұ��� �ϴٸ� ĳ���Ϳ��� ������ �˷��ִ� �Լ�
bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	//BYTE itemType = item->GetType();
	//BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break; // TODO: ������ ������ ���밡�ɿ��� ó��
#endif
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���� ������ �� �����ϴ�."));
					return false;
				}
				break;

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"�ٷ��� ���� ������ �� �����ϴ�."));
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"������ ���� ������ �� �����ϴ�."));
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ø�� ���� ������ �� �����ϴ�."));
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"ü���� ���� ������ �� �����ϴ�."));
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item))
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			|| (GetWear (WEAR_COSTUME_MOUNT) && GetWear (WEAR_COSTUME_MOUNT)->IsSameSpecialGroup (item))
#endif
			)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"���� ������ ����ũ ������ �� ���� ���ÿ� ������ �� �����ϴ�."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(GetLanguage(),"��ȥ���� ���� ���¿��� ������ ������ �� �����ϴ�."));
			return false;
		}

	}

	return true;
}

/// ���� ĳ������ ���¸� �������� ���� ���� item�� ���� �� �ִ� �� Ȯ���ϰ�, �Ұ��� �ϴٸ� ĳ���Ϳ��� ������ �˷��ִ� �Լ�
bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{

	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "��Ʈ �κ��丮�� �������� �����ϸ� ������ �� �����ϴ�.");

	// ������ ������ �� ���� ������
	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (item->GetType() == ITEM_WEAPON)
	{
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}
	// ������ unequip�� �κ��丮�� �ű� �� �� �ڸ��� �ִ� �� Ȯ��
	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG( -1 == pos, "����ǰ�� �� ������ �����ϴ�." );
	}


	return true;
}



#ifdef PRIVATESHOP_SEARCH_SYSTEM
#include "entity.h"
#include "offlineshop.h"
#include <boost/unordered_map.hpp>

struct FFindShopSearch
{
	std::map<DWORD, LPCHARACTER> m_mapPcShop;
	LPCHARACTER m_pChar;

	FFindShopSearch(LPCHARACTER pChar) : m_pChar(pChar)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (NULL == ent)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (m_pChar == pChar)
				return;

			if (pChar->IsOfflineShopNPC() && pChar->GetRaceNum() == 30000)
				m_mapPcShop[(DWORD)pChar->GetVID()] = pChar;
		}
	}
};

bool CompareItemVnumAcPriceAC(COfflineShop::OFFLINE_SHOP_ITEM i, COfflineShop::OFFLINE_SHOP_ITEM j)
{
	return (i.vnum < j.vnum) && (i.price < j.price);
}

#ifdef ENABLE_CHEQUE_SYSTEM
void CHARACTER::ShopSearchInfo(bool bNameOnly, BYTE bItemType, BYTE bItemSubType, long lMinGold, long lMaxGold, int bMinCheque, int bMaxCheque, const char* c_szItemName)
#else
void CHARACTER::ShopSearchInfo(bool bNameOnly, BYTE bItemType, BYTE bItemSubType, long lMinGold, long lMaxGold, const char* c_szItemName)
#endif
{
	if (!IsPC() || IsDead())
		return;

	if (IsOpenSafebox() || GetShop() || IsCubeOpen() || IsDead() || GetExchange() || GetOfflineShop() || GetMyShop())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Sa fii sigur ca nu ai ferestre deschise!"));
		return;
	}

	if (quest::CQuestManager::instance().GetEventFlag("disable_shop_search"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Temporar dezactivat searchshopul."));
		return;
	}
	/*
	if (!FindSpecifyItem(60004) && !FindSpecifyItem(60005))
	{
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ticaret camina sahip olmadan arama yapamazsin."));
	return;
	}
	*/
	// if (bJob < JOB_WARRIOR || bJob > JOB_SHAMAN)
		// return;
	if (bItemType != 99)
		if (bItemType < ITEM_NONE || bItemType > ITEM_BELT)
			return;
		
	if (bItemSubType != 99)
		if (bItemSubType < USE_POTION || bItemSubType > USE_PUT_INTO_RING_SOCKET)
			return;
	// if (bMinLevel < 0 || bMinLevel > PLAYER_MAX_LEVEL_CONST)
		// return;
	// if (bMaxLevel < 0 || bMaxLevel > PLAYER_MAX_LEVEL_CONST)
		// return;
	// if (bMinRefine < 0 || bMinRefine > 9)
		// return;
	// if (bMaxRefine < 0 || bMaxRefine > 9)
		// return;
	if (lMinGold < 0 || lMinGold > GOLD_MAX)
		return;
	if (lMaxGold < 0 || lMaxGold > GOLD_MAX)
		return;
	if (bMinCheque < 0 || bMinCheque > CHEQUE_MAX)
		return;
	if (bMaxCheque < 0 || bMaxCheque > CHEQUE_MAX)
		return;
	// if (bMinLevel > bMaxLevel)
		// return;
	// if (bMinRefine > bMaxRefine)
		// return;
	if (lMinGold > lMaxGold)
		return;
	if (bMinCheque > bMaxCheque)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

	if (!pPC)
		return;

	DWORD dwShopSearchSecCycle = 3;
	DWORD dwNowSec = get_global_time();
	DWORD dwLastShopSearchAttrSec = pPC->GetFlag("ShopSearch.LastShopSearchSecAttr");

	if (dwLastShopSearchAttrSec + dwShopSearchSecCycle > dwNowSec)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Esti prea rapid! Te rog asteapta %d s."), (dwShopSearchSecCycle, dwShopSearchSecCycle - (dwNowSec - dwLastShopSearchAttrSec)));
		return;
	}

	pPC->SetFlag("ShopSearch.LastShopSearchSecAttr", dwNowSec);

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

	if (!pMap)
		return;

	FFindShopSearch f(this);
	pMap->for_each(f);

	if (f.m_mapPcShop.empty())
		return;

	std::vector<COfflineShop::OFFLINE_SHOP_ITEM> sendItems;

	for (std::map<DWORD, LPCHARACTER>::iterator iter = f.m_mapPcShop.begin(); iter != f.m_mapPcShop.end(); ++iter)
	{
		LPCHARACTER tch = iter->second;

		if (!tch)
			continue;

		if (!tch->IsOfflineShopNPC() || !tch->GetOfflineShop())
			continue;
		
		// Check if is your shop;
		// if (tch->GetName() == GetName())
			// continue;

		std::vector<COfflineShop::OFFLINE_SHOP_ITEM> offlineShopItem = tch->GetOfflineShop()->GetItemVector();
		for (std::vector<COfflineShop::OFFLINE_SHOP_ITEM>::iterator iter2 = offlineShopItem.begin(); iter2 != offlineShopItem.end(); ++iter2)
		{
			if (iter2->bIsSold)
				continue;
			
			if (iter2->vnum < 1)
				continue;

			if (bNameOnly)
			{
				std::string strItemName(c_szItemName);
				// if (strItemName.length() > 2)
				// {
					// if (!((iter2->bItemLevel >= bMinLevel && iter2->bItemLevel <= bMaxLevel) || iter2->bItemLevel == 0))
						// continue;

					const TItemTable * item_table = ITEM_MANAGER::Instance().GetTable(iter2->vnum);
					
					if (!item_table)
						continue;

					// Item name
					std::string ProtoName = item_table->szLocaleName[GetLanguageForSearchShop()];
					for(itertype(ProtoName) iter = ProtoName.begin(); iter != ProtoName.end(); iter++){
						*iter = tolower(*iter);
					}
					
					for(itertype(strItemName) iter = strItemName.begin(); iter != strItemName.end(); iter++){
						*iter = tolower(*iter);
					}
					
					bool namePass = (strItemName.length() ? strstr(ProtoName.c_str(), strItemName.c_str()) != NULL : true);
					
					// test tes testt
					// for (int i = 0; i < 6; ++i)
					// ChatPacket(CHAT_TYPE_INFO, "Lang :%d - Name: %s (My lang is)", i, item_table->szLocaleName[i], GetLanguage());

					if (!(iter2->price >= lMinGold && iter2->price <= lMaxGold))
						continue;
					
					// TYPE || SUBTYPE CHECK
					// if (bItemSubType != 0)
					// {
						// if (bItemSubType != 0)
							// if (iter2->bItemSubType != bItemSubType)
								// continue;
					// }
						
					// if (bItemSubType != 99)
					// {
						// if (iter2->bItemType != bItemType)
							// continue;
					// }
				
					// TYPE || SUBTYPE CHECK

#ifdef ENABLE_CHEQUE_SYSTEM
					if (!(iter2->cheque >= bMinCheque && iter2->cheque <= bMaxCheque))
						continue;
#endif

					// bool pushback = false;

					// std::string foundName(iter2->szItemName);

					// if (foundName.find(strItemName) != std::string::npos)
						// pushback = true;

					if (namePass)
					{
						iter2->dwShopVID = iter->first;
						strncpy(iter2->szOwnerName, tch->GetName(), CHARACTER_NAME_MAX_LEN);
						sendItems.push_back(*iter2);
					}
				// }
			}
			else
			{
				bool bCanContinueSubType = false;
				// bool bCanContinueType = false;
				
				// if (iter2->bItemType == bItemType)
						// bCanContinueType = true;
				
				if (bItemSubType == 99 || iter2->bItemSubType == bItemSubType)
					bCanContinueSubType = true;

				// ChatPacket(CHAT_TYPE_INFO, "Type: %d SubType:%d > TypeNeed: %d SubTypeNeed: %d", iter2->bItemType, iter2->bItemSubType, bItemType, bItemSubType);
					
				
				if (iter2->bItemType == bItemType && bCanContinueSubType || bItemType == 99)
				{
					// if ((iter2->bItemType == ITEM_WEAPON || iter2->bItemType == ITEM_ARMOR) && !(iter2->bItemRefine >= bMinRefine && iter2->bItemRefine <= bMaxRefine))
						// continue;

					// if (!((iter2->bItemLevel >= bMinLevel && iter2->bItemLevel <= bMaxLevel) || iter2->bItemLevel == 0))
						// continue;
					
					// ChatPacket(CHAT_TYPE_INFO, "asd");

					if (!(iter2->price >= lMinGold && iter2->price <= lMaxGold))
						continue;

#ifdef ENABLE_CHEQUE_SYSTEM
					if (!(iter2->cheque >= bMinCheque && iter2->cheque <= bMaxCheque))
						continue;
#endif

					// bool cont = false;
					// switch (bJob)
					// {
					// case JOB_WARRIOR:
						// if (iter2->dwAntiFlag & ITEM_ANTIFLAG_WARRIOR)
							// cont = true;
						// break;

					// case JOB_ASSASSIN:
						// if (iter2->dwAntiFlag & ITEM_ANTIFLAG_ASSASSIN)
							// cont = true;
						// break;

					// case JOB_SHAMAN:
						// if (iter2->dwAntiFlag & ITEM_ANTIFLAG_SHAMAN)
							// cont = true;
						// break;

					// case JOB_SURA:
						// if (iter2->dwAntiFlag & ITEM_ANTIFLAG_SURA)
							// cont = true;
						// break;
					// }

					// if (cont)
						// continue;

					iter2->dwShopVID = iter->first;
					strncpy(iter2->szOwnerName, tch->GetName(), CHARACTER_NAME_MAX_LEN);
					sendItems.push_back(*iter2);

				}
			}
		}
	}

	std::stable_sort(sendItems.begin(), sendItems.end(), CompareItemVnumAcPriceAC);

	for (std::vector<COfflineShop::OFFLINE_SHOP_ITEM>::iterator iter = sendItems.begin(); iter != sendItems.end(); ++iter)
	{
		if (!IsPC() || !iter->dwShopVID)
			continue;

		if (iter->vnum < 1 || iter->bIsSold)
			continue;

		TPacketGCShopSearchItemInfo pack;
		pack.bHeader = HEADER_GC_PSHOP_SEARCH_ITEM_INFO;

		pack.dwVID = iter->dwShopVID;
		strlcpy(pack.szOwnerName, iter->szOwnerName, CHARACTER_NAME_MAX_LEN);
		pack.bPos = iter->pos;

		pack.dwGold = iter->price;

#ifdef ENABLE_CHEQUE_SYSTEM
		pack.bCheque = iter->cheque;
#endif
		pack.dwVnum = iter->vnum;
		pack.bCount = static_cast<BYTE>(iter->count);
		pack.dwFlags = iter->dwFlag;
		pack.dwAntiFlags = iter->dwAntiFlag;

		thecore_memcpy(pack.alSockets, iter->alSockets, sizeof(pack.alSockets));
		thecore_memcpy(pack.aAttr, iter->aAttr, sizeof(pack.aAttr));

#ifdef CHANGELOOK_SYSTEM
		pack.dwTransmutation = iter->transmutation;
#endif
		GetDesc()->LargePacket(&pack, sizeof(TPacketGCShopSearchItemInfo));
	}
}

void CHARACTER::ShopSearchBuyItem(DWORD dwShopVID, BYTE bItemPos)
{
	if (!IsPC() || IsDead())
		return;

	if (quest::CQuestManager::instance().GetEventFlag("disable_shop_search"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Fereastra de cautare a articolelor este in prezent dezactivata."));
		return;
	}

	if (IsOpenSafebox() || GetShop() || IsCubeOpen() || IsDead() || GetExchange() || GetOfflineShop() || GetMyShop())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Fii sigur ca nu ai ferestre deschise!"));
		return;
	}
	/*
	if (!FindSpecifyItem(60005))
	{
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Premium ticaret cami olmadan satin alma islemi yapamassin."));
	return;
	}
	*/
	LPCHARACTER pkChrShop = CHARACTER_MANAGER::instance().Find(dwShopVID);

	if (!pkChrShop)
		return;

	LPOFFLINESHOP pkShop = pkChrShop->GetOfflineShop();

	if (!pkShop)
		return;

	if (GetMapIndex() != pkChrShop->GetMapIndex())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nu esti pe aceeasi harta cu piata. Cumparaturile au esuat."));
		return;
	}

	SetOfflineShopOwner(pkChrShop);
	int32_t returnHeader = pkShop->Buy(this, bItemPos, true);
	SetOfflineShopOwner(NULL);

	if (SHOP_SUBHEADER_GC_OK == returnHeader)
		ChatPacket(CHAT_TYPE_COMMAND, "ShopSearchBuy");
	else
		ChatPacket(CHAT_TYPE_COMMAND, "ShopSearchError %d", returnHeader);
}
#endif