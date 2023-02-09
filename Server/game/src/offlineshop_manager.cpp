#include "stdafx.h"
#include <boost/algorithm/string/predicate.hpp>
#include <cctype>
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "offlineshop.h"
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
#include "desc_client.h"
#include "group_text_parse_tree.h"
#include "offlineshop_manager.h"
#include "p2p.h"
#include "entity.h"
#include "sectree_manager.h"

std::set<DWORD> s_set_dwOfflineShop;

COfflineShopManager::COfflineShopManager()
{
}

COfflineShopManager::~COfflineShopManager()
{
}

LPOFFLINESHOP COfflineShopManager::CreateOfflineShop(LPCHARACTER npc, DWORD dwOwnerPID)
{
	if (FindOfflineShop(npc->GetVID()))
		return NULL;

	LPOFFLINESHOP pkOfflineShop = M2_NEW COfflineShop;
	pkOfflineShop->SetOfflineShopNPC(npc);

	m_map_pkOfflineShopByNPC.insert(TShopMap::value_type(npc->GetVID(), pkOfflineShop));
	m_Map_pkOfflineShopByNPC2.insert(TOfflineShopMap::value_type(dwOwnerPID, npc->GetVID()));

	return pkOfflineShop;
}

LPOFFLINESHOP COfflineShopManager::FindOfflineShop(DWORD dwVID)
{
	TShopMap::iterator it = m_map_pkOfflineShopByNPC.find(dwVID);

	if (it == m_map_pkOfflineShopByNPC.end())
		return NULL;

	return it->second;
}

DWORD COfflineShopManager::FindMyOfflineShop(DWORD dwPID)
{
	TOfflineShopMap::iterator it = m_Map_pkOfflineShopByNPC2.find(dwPID);
	if (m_Map_pkOfflineShopByNPC2.end() == it)
		return 0;

	return it->second;
}

bool COfflineShopManager::MapCheck(DWORD mapIndex, DWORD empire)
{
	if (empire == 1)
	{
		if (mapIndex == 1)
			return true;
	}

	if (empire == 2)
	{
		if (mapIndex == 1)
			return true;
	}

	if (empire == 3)
	{
		if (mapIndex == 1)
			return true;
	}

	return false;
}

bool COfflineShopManager::ChannelCheck(DWORD dwChannel)
{
	if (dwChannel == 98)
		return true;

	return false;
}

bool COfflineShopManager::SearchOfflineShop(LPCHARACTER ch)
{
	if (!ch)
		return false;

	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (npc)
		return true;
	else
		return false;
}

void COfflineShopManager::ResetOfflineShopStatus(LPCHARACTER ch)
{
	if (ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		//sys_log(0, "COfflineShopManager::ResetOfflineShopStatus Affect found checking... Name: [%s]", ch->GetName());

		if (!SearchOfflineShop(ch))
		{
			ch->RemoveAffect(AFFECT_SHOPOWNER);
			//sys_log(0, "COfflineShopManager::ResetOfflineShopStatus shop wasn't exist removed shopowner affect name: [%s]", ch->GetName());
		}
	}
}

bool COfflineShopManager::HaveOfflineShopOnAccount(DWORD aID)
{
	if (s_set_dwOfflineShop.find(aID) != s_set_dwOfflineShop.end())
		return true;

	return false;
}

void COfflineShopManager::InsertOfflineShopToAccount(DWORD aID)
{
	s_set_dwOfflineShop.insert(aID);
}

void COfflineShopManager::BootShop(DWORD idAcc, DWORD dwOwner)
{
	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(dwOwner));

	if (!pkOfflineShop)
		return;
	
	pkOfflineShop->PutItemsBoot(idAcc, dwOwner);
}

void COfflineShopManager::DeleteOfflineShopOnAccount(DWORD aID)
{
	itertype(s_set_dwOfflineShop) iter = s_set_dwOfflineShop.find(aID);

	if (iter != s_set_dwOfflineShop.end())
		s_set_dwOfflineShop.erase(iter);
	else
		sys_err("DeleteOfflineShopOnAccount can't find Account ID: %d possible bug!");
}

bool COfflineShopManager::StartShopping(LPCHARACTER pkChr, LPCHARACTER pkChrShopKeeper)
{
	if (!pkChr || pkChrShopKeeper)
		return false;

	if (pkChr->GetOfflineShopOwner() == pkChrShopKeeper)
		return false;

	if (pkChrShopKeeper->IsPC())
		return false;

	sys_log(0, "OFFLINE_SHOP: START: %s", pkChr->GetName());

	return true;
}

void COfflineShopManager::StopShopping(LPCHARACTER ch)
{
	if (!ch)
		return;

	LPOFFLINESHOP pkOfflineShop;

	if (!(pkOfflineShop = ch->GetOfflineShop()))
		return;

	pkOfflineShop->RemoveGuest(ch);
	sys_log(0, "OFFLINE_SHOP: END: %s", ch->GetName());
}

void COfflineShopManager::Buy(LPCHARACTER ch, BYTE pos)
{
	if (!ch)
		return;

	if (!ch->GetOfflineShop())
		return;

	if (!ch->GetOfflineShopOwner())
		return;

	if (DISTANCE_APPROX(ch->GetX() - ch->GetOfflineShopOwner()->GetX(), ch->GetY() - ch->GetOfflineShopOwner()->GetY()) > 5000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 살 수 없습니다."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop = ch->GetOfflineShop();

	if (!pkOfflineShop)
		return;

	int ret = pkOfflineShop->Buy(ch, pos);

	// The result is not equal to SHOP_SUBHEADER_GC_OK, send the error to the character.
	if (SHOP_SUBHEADER_GC_OK != ret)
	{
		TPacketGCShop pack;
		pack.header = HEADER_GC_OFFLINE_SHOP;
		pack.subheader = ret;
		pack.size = sizeof(TPacketGCShop);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}
}

// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShopManager::PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice, int bCheque)
#else
void COfflineShopManager::PutItem(LPCHARACTER ch, TItemPos item_pos, int llPrice)
#endif
// END_OF_MAX_YANG
{
	// BEGIN_MAX_YANG
	if (!ch || llPrice <= 0 || llPrice > GOLD_MAX)
		return;
	// END_OF_MAX_YANG

#ifdef ENABLE_CHEQUE_SYSTEM
	if (bCheque > CHEQUE_MAX)
		return;
#endif

	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attendi per completare l'azione."));
		return;
	}

	ch->SetMyOfflineShopTime();

	quest::PC * pPC = quest::CQuestManager::Instance().GetPC(ch->GetPlayerID());
	if (pPC->IsRunning())
		return;

	if (ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Assicurati che tu non abbia un altra finestra aperta."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo in Capitale."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Non puoi farlo in questo Channel."));
		return;
	}

	if (!ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai bisogno di un negozio attivo per farlo."));
		return;
	}

	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	pkOfflineShop->PutItem(ch, item_pos, llPrice, bCheque);
#else
	pkOfflineShop->PutItem(ch, item_pos, llPrice);
#endif
	// END_OF_MAX_YANG
}

// FUNCTIONS_EXTRA_BEGIN

// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShopManager::PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice, int bCheque)
#else
void COfflineShopManager::PutItemPos(LPCHARACTER ch, TItemPos item_pos, int iPos, int llPrice)
#endif
// END_OF_MAX_YANG
{
	// BEGIN_MAX_YANG
	if (!ch || llPrice <= 0 || llPrice > GOLD_MAX)
		return;
	// END_OF_MAX_YANG

#ifdef ENABLE_CHEQUE_SYSTEM
	if (bCheque > CHEQUE_MAX)
		return;
#endif

	quest::PC * pPC = quest::CQuestManager::Instance().GetPC(ch->GetPlayerID());
	if (pPC->IsRunning())
		return;

	if (ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Assicurati che tu non abbia un altra finestra aperta."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Non puoi farlo in questo Channel."));
		return;
	}

	if (!ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai bisogno di un negozio attivo per farlo."));
		return;
	}

	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	pkOfflineShop->PutItemPos(ch, item_pos, iPos, llPrice, bCheque);
#else
	pkOfflineShop->PutItemPos(ch, item_pos, iPos, llPrice);
#endif
	// END_OF_MAX_YANG
}

// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
void COfflineShopManager::EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice, int bCheque)
#else
void COfflineShopManager::EditPriceItem(LPCHARACTER ch, int item_pos, int llPrice)
#endif
// END_OF_MAX_YANG
{
	// BEGIN_MAX_YANG
	if (!ch || llPrice <= 0 || llPrice > GOLD_MAX)
		return;
	// END_OF_MAX_YANG

#ifdef ENABLE_CHEQUE_SYSTEM
	if (bCheque > CHEQUE_MAX)
		return;
#endif

	if (thecore_pulse() - ch->GetMyOfflineShopTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attendi per completare l'azione."));
		return;
	}

	ch->SetMyOfflineShopTime();

	quest::PC * pPC = quest::CQuestManager::Instance().GetPC(ch->GetPlayerID());
	if (pPC->IsRunning())
		return;

	if (ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Assicurati che tu non abbia un altra finestra aperta."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo in Capitale."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Non puoi farlo in questo Channel."));
		return;
	}

	if (!ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai bisogno di un negozio attivo per farlo."));
		return;
	}

	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
	pkOfflineShop->EditPriceItem(ch, item_pos, llPrice, bCheque);
#else
	pkOfflineShop->EditPriceItem(ch, item_pos, llPrice);
#endif
	// END_OF_MAX_YANG
}

void COfflineShopManager::SendItemsEditMode(LPCHARACTER ch)
{
	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	pkOfflineShop->SendInfoItems(ch, npc);
	
	// Set status for player while edit shop
	ch->SetEditOfflineShopMode(true);
}

void COfflineShopManager::DelSingleItem(LPCHARACTER ch, int item_pos)
{
	quest::PC * pPC = quest::CQuestManager::Instance().GetPC(ch->GetPlayerID());
	if (pPC->IsRunning())
		return;

	if (ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Assicurati che tu non abbia un altra finestra aperta."));
		return;
	}

	if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
		return;
	}

	if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Non puoi farlo in questo Channel."));
		return;
	}

	if (!ch->IsAffectFlag(AFF_SHOPOWNER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai bisogno di un negozio attivo per fare questo."));
		return;
	}

	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
		return;
	}

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	pkOfflineShop->DelSingleItem(ch, item_pos);
}

//FUNCTIONS_EXTRA_END

void COfflineShopManager::AutoCloseOfflineShop(LPCHARACTER ch)
{
	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

	if (!npc)
		return;

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

	if (!pkOfflineShop)
		return;

	pkOfflineShop->GiveBackMyItems(ch);
	pkOfflineShop->Destroy(npc);

	m_map_pkOfflineShopByNPC.erase(npc->GetVID());
	m_Map_pkOfflineShopByNPC2.erase(npc->GetOfflineShopRealOwner());
	DeleteOfflineShopOnAccount(ch->GetDesc()->GetAccountTable().id);
	M2_DELETE(pkOfflineShop);
	ch->RemoveAffect(AFFECT_SHOPOWNER);

	// Close Shop Offlne
	TPacketOfflineShopDestroy pCloseShop;
	pCloseShop.dwOwnerID = ch->GetPlayerID();
	db_clientdesc->DBPacket(HEADER_GD_OFFLINESHOP_DESTROY, 0, &pCloseShop, sizeof(pCloseShop));
	// Close Shop Offlne

	// BEGIN_COUNTER_UPDATE
	TPacketUpdateOfflineShopsCount pCount;
	pCount.bIncrease = false;
	db_clientdesc->DBPacket(HEADER_GD_UPDATE_OFFLINESHOP_COUNT, 0, &pCount, sizeof(pCount));
	// END_OF_COUNTER_UPDATE
	
}

void COfflineShopManager::AutoCloseOfflineShopBoot(DWORD idAcc, DWORD dwPlayerID)
{
	LPCHARACTER npc;
	npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(dwPlayerID));

	if (!npc)
		return;

	LPOFFLINESHOP pkOfflineShop;
	pkOfflineShop = FindOfflineShop(FindMyOfflineShop(dwPlayerID));

	if (!pkOfflineShop)
		return;

	m_map_pkOfflineShopByNPC.erase(npc->GetVID());
	m_Map_pkOfflineShopByNPC2.erase(npc->GetOfflineShopRealOwner());
	
	pkOfflineShop->Destroy(npc);
	
	DeleteOfflineShopOnAccount(idAcc);
	
	M2_DELETE(pkOfflineShop);

	// Close Shop Offlne
	TPacketOfflineShopDestroy pCloseShop;
	pCloseShop.dwOwnerID = dwPlayerID;
	db_clientdesc->DBPacket(HEADER_GD_OFFLINESHOP_DESTROY, 0, &pCloseShop, sizeof(pCloseShop));
	// Close Shop Offlne	

	// BEGIN_COUNTER_UPDATE
	TPacketUpdateOfflineShopsCount pCount;
	pCount.bIncrease = false;
	db_clientdesc->DBPacket(HEADER_GD_UPDATE_OFFLINESHOP_COUNT, 0, &pCount, sizeof(pCount));
	// END_OF_COUNTER_UPDATE
}

void COfflineShopManager::DestroyOfflineShop(LPCHARACTER ch, DWORD dwVID, DWORD dwPlayerID, bool pcMode)
{
	if (pcMode)
	{
		if (!ch)
			return;

		quest::PC * pPC = quest::CQuestManager::Instance().GetPC(ch->GetPlayerID());

		if (pPC->IsRunning())
			return;

		if (ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetOfflineShop())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Assicurati che tu non abbia un altra finestra aperta."));
			return;
		}

		if (!COfflineShopManager::instance().MapCheck(ch->GetMapIndex(), ch->GetEmpire()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Puoi aprire il menu' solo dalla Capitale."));
			return;
		}

		if (!COfflineShopManager::instance().ChannelCheck(g_bChannel))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Offline Shop> Non puoi farlo in questo Channel."));
			return;
		}

		if (!ch->IsAffectFlag(AFF_SHOPOWNER))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai un negozio da chiudere."));
			return;
		}

		LPCHARACTER npc;
		npc = CHARACTER_MANAGER::instance().Find(FindMyOfflineShop(ch->GetPlayerID()));

		if (!npc)
			return;

		LPOFFLINESHOP pkOfflineShop;
		pkOfflineShop = FindOfflineShop(FindMyOfflineShop(ch->GetPlayerID()));

		if (!pkOfflineShop)
			return;

		if (DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) > 16000)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Devi essere vicino al tuo negozio."));
			return;
		}
		
		// Prevent copy item, while shop start destroy event
		// pkOfflineShop->SetOwnerManage(true);
		// pkOfflineShop->SetClosed(true);
		
		// if (pkOfflineShop->GetIsManage() == true)
			// return;
		
		// Prevent copy item, while shop start destroy event

		//LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, "", "DESTROY");
		pkOfflineShop->GiveBackMyItems(ch);

		m_map_pkOfflineShopByNPC.erase(npc->GetVID());
		m_Map_pkOfflineShopByNPC2.erase(npc->GetOfflineShopRealOwner());
		
		pkOfflineShop->Destroy(npc);
		
		DeleteOfflineShopOnAccount(ch->GetDesc()->GetAccountTable().id);
		M2_DELETE(pkOfflineShop);
		ch->RemoveAffect(AFFECT_SHOPOWNER);

		// Close Shop Offlne
		TPacketOfflineShopDestroy pCloseShop;
		pCloseShop.dwOwnerID = ch->GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_OFFLINESHOP_DESTROY, 0, &pCloseShop, sizeof(pCloseShop));
		// Close Shop Offlne		

		// BEGIN_COUNTER_UPDATE
		TPacketUpdateOfflineShopsCount pCount;
		pCount.bIncrease = false;
		db_clientdesc->DBPacket(HEADER_GD_UPDATE_OFFLINESHOP_COUNT, 0, &pCount, sizeof(pCount));
		// END_OF_COUNTER_UPDATE
	}
	else
	{
		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(dwVID);
		LPOFFLINESHOP pkOfflineShop = FindOfflineShop(dwVID);

		if (!npc)
		{
			sys_err("COfflineShopManager::DestroyOfflineShop CRITICAL ERROR! NPC NOT FOUND! VID: %d", dwVID);
			return;
		}

		if (!pkOfflineShop)
		{
			sys_err("COfflineShopManager::DestroyOfflineShop CRITICAL ERROR! pkOfflineShop NOT FOUND! VID: %d", dwVID);
			return;
		}
		
		// if someone try to buy something while stop event destroy, stop him
		// pkOfflineShop->SetOwnerManage(true);
		
		// PREVENT_COPY_ITEM
		// pkOfflineShop->SetClosed(true);
		// pkOfflineShop->SetClosedAuto(true);
		// PREVENT_COPY_ITEM

		// Close Shop Offlne
		TPacketOfflineShopDestroy pCloseShop;
		pCloseShop.dwOwnerID = dwPlayerID;
		db_clientdesc->DBPacket(HEADER_GD_OFFLINESHOP_DESTROY, 0, &pCloseShop, sizeof(pCloseShop));
		// Close Shop Offlne		

		m_map_pkOfflineShopByNPC.erase(npc->GetVID());
		m_Map_pkOfflineShopByNPC2.erase(npc->GetOfflineShopRealOwner());
		//LogManager::instance().OfflineShopLog(npc->GetOfflineShopRealOwnerAccountID(), "", "DESTROY");
		DeleteOfflineShopOnAccount(npc->GetOfflineShopRealOwnerAccountID());
		M2_DELETE(pkOfflineShop);

		// BEGIN_COUNTER_UPDATE
		TPacketUpdateOfflineShopsCount pCount;
		pCount.bIncrease = false;
		db_clientdesc->DBPacket(HEADER_GD_UPDATE_OFFLINESHOP_COUNT, 0, &pCount, sizeof(pCount));
		// END_OF_COUNTER_UPDATE
	}
}

void COfflineShopManager::FetchMyItems(LPCHARACTER ch)
{
	if (!ch)
		return;

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


		" FROM %soffline_shop_item WHERE owner_id = %u", get_table_postfix(), ch->GetPlayerID());

	std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));

	if (pMsg->Get()->uiNumRows == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai nulla da ritirare."));
		return;
	}

	MYSQL_ROW row;
	while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		TPlayerItem item;
		int col = 0;
		
		str_to_number(item.pos, row[col++]);
		str_to_number(item.count, row[col++]);

		str_to_number(item.vnum, row[col++]);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			str_to_number(item.alSockets[i], row[col++]);
		}

#ifdef CHANGELOOK_SYSTEM
		str_to_number(item.transmutation, row[col++]);
#endif

		for (int i = 0, iStartAttributeType = 7, iStartAttributeValue = iStartAttributeType + 1, iStartFrozenValue = iStartAttributeValue + 1; i < ITEM_ATTRIBUTE_MAX_NUM; ++i, iStartAttributeType += 3, iStartAttributeValue += 3, iStartFrozenValue += 3)
		{
			str_to_number(item.aAttr[i].bType, row[col++]);
			str_to_number(item.aAttr[i].sValue, row[col++]);


#ifdef __FROZENBONUS_SYSTEM__
			str_to_number(item.aAttr[i].isFrozen, row[col++]);
#endif
		}

		LPITEM pItem = ITEM_MANAGER::instance().CreateItem(item.vnum, item.count);

		if (pItem)
		{
			pItem->SetSockets(item.alSockets);
			pItem->SetAttributes(item.aAttr);





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
					pItem->AddToCharacter(ch, TItemPos(INVENTORY, cell));
				// END_OF_FOR_DRAGON_SOUL
			}
			else
			{
/*				pItem->AddToGround(ch->GetMapIndex(), ch->GetXYZ());
				pItem->StartDestroyEvent();
				pItem->SetOwnership(ch, g_iOfflineShopOwnerShipTime);*/
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai abbastanza spazio nello inventario"));
				return;
			}

			// release DB
			DBManager::instance().DirectQuery("DELETE FROM %soffline_shop_item WHERE owner_id = %u AND pos = %d AND vnum = %d LIMIT 1", get_table_postfix(), ch->GetPlayerID(), item.pos, item.vnum);

			/* log for web */
			LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pItem->GetName(), "FETCH");
			/* end log for web */
		}
	}
}

bool COfflineShopManager::WithdrawAllMoney(LPCHARACTER ch)
{
	if (!ch)
		return false;

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT gold_offlineshop FROM player.player WHERE id = %u", ch->GetPlayerID()));

	if (pMsg->Get()->uiNumRows == 0)
		return false;

	DWORD dwBankMoney = 0, dwMoneyToGet = 0;
	bool bGiveLittleMoney = false; // for prevent bug GOLD_MAX in the bank

	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	str_to_number(dwBankMoney, row[0]);

	if (dwBankMoney <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai nulla da ricevere."));
		return false;
	}

	if (dwBankMoney > GOLD_MAX)
	{
		bGiveLittleMoney = true;
		dwMoneyToGet = 1000000000; // 1.000.000.000 golds
	}
	else
	{
		dwMoneyToGet = dwBankMoney;
	}

	bool isOverFlow = ch->GetGold() + dwMoneyToGet > GOLD_MAX - 1 ? true : false;

	if (isOverFlow)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai troppi soldi nel tuo inventario."));
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 3"));
		return false;
	}

	ch->PointChange(POINT_GOLD, dwMoneyToGet, false);
	DBManager::instance().DirectQuery("UPDATE player.player SET gold_offlineshop = gold_offlineshop - %u WHERE id = %u", dwMoneyToGet, ch->GetPlayerID());

	if (bGiveLittleMoney)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 4"));
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 5"));
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai ricevuto: %u"), dwMoneyToGet);

	char pszConv[32];
	sprintf(pszConv, "%d", dwMoneyToGet);

	LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pszConv, "FETCH_MONEY");

	return true;
}

#ifdef ENABLE_CHEQUE_SYSTEM
bool COfflineShopManager::WithdrawAllCheque(LPCHARACTER ch)
{
	if (!ch)
		return false;

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT cheque_offlineshop FROM player.player WHERE id = %u", ch->GetPlayerID()));

	if (pMsg->Get()->uiNumRows == 0)
		return false;

	DWORD dwBankChequeUmut = 0, dwMax = 0;
	bool bGiveLittleMoney = false;

	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	str_to_number(dwBankChequeUmut, row[0]);

	if (dwBankChequeUmut <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Non hai nulla da ritirare."));
		return false;
	}

	if (dwBankChequeUmut > 998)
	{
		bGiveLittleMoney = true;
		dwMax = 998; // 200'den fazla giremen bebe?m
	}
	else
	{
		dwMax = dwBankChequeUmut;
	}

	bool isOverFlow = ch->GetCheque() + dwMax > 998 - 1 ? true : false;

	if (isOverFlow)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You have too many Won in your inventory.");
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 3"));
		return false;
	}

	ch->PointChange(POINT_CHEQUE, dwMax, false);

	DBManager::instance().DirectQuery("UPDATE player.player SET cheque_offlineshop = cheque_offlineshop - %u WHERE id = %u", dwMax, ch->GetPlayerID());

	if (bGiveLittleMoney)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 4"));
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Bank text 5"));
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Hai ricevuto: %u"), dwMax);

	char pszConv[32];
	sprintf(pszConv, "%d", dwMax);

	LogManager::instance().OfflineShopLog(ch->GetDesc()->GetAccountTable().id, pszConv, "FETCH_MONEY");

	return true;
}
#endif

void COfflineShopManager::RefreshMoney(LPCHARACTER ch)
{
	if (!ch)
		return;

#ifdef ENABLE_CHEQUE_SYSTEM
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT gold_offlineshop, cheque_offlineshop FROM player.player WHERE id = %u", ch->GetPlayerID()));
#else
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT gold_offlineshop FROM player.player WHERE id = %u", ch->GetPlayerID()));
#endif

	TPacketGCShop p;
	TPacketGCOfflineShopMoney p2;

	p.header = HEADER_GC_OFFLINE_SHOP;
	p.subheader = SHOP_SUBHEADER_CG_REFRESH_OFFLINE_SHOP_MONEY;

	if (pMsg->Get()->uiNumRows == 0)
	{
		p2.llMoney = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
		p2.bCheque = 0;
#endif
		p.size = sizeof(p) + sizeof(p2);
		if (ch->GetDesc())
		{
			ch->GetDesc()->BufferedPacket(&p, sizeof(TPacketGCShop));
			ch->GetDesc()->Packet(&p2, sizeof(TPacketGCOfflineShopMoney));
		}
	}
	else
	{
		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		str_to_number(p2.llMoney, row[0]);
#ifdef ENABLE_CHEQUE_SYSTEM
		str_to_number(p2.bCheque, row[1]);
#endif
		p.size = sizeof(p) + sizeof(p2);
		if (ch->GetDesc())
		{
			ch->GetDesc()->BufferedPacket(&p, sizeof(TPacketGCShop));
			ch->GetDesc()->Packet(&p2, sizeof(TPacketGCOfflineShopMoney));
		}
	}
}



