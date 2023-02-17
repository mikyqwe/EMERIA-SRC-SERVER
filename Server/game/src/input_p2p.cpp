#include "stdafx.h"
#include "../../common/billing.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "dev_log.h"
#include "locale_service.h"
#include "questmanager.h"
#include "pcbang.h"
#include "skill.h"
#include "threeway_war.h"
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
#include "crc32.h"
#endif

#ifdef OFFLINE_SHOP
#include "offlineshop_manager.h"
#include "sectree_manager.h"
#include "buffer_manager.h"
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"
#endif

#include "../../common/CommonDefines.h"
#ifdef ENABLE_DECORUM
#include "decorum_arena.h"
#include "decorum_manager.h"
#endif
////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char * c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char * c_pData)
{
	TPacketGGLogout * p = (TPacketGGLogout *) c_pData;

#ifdef ENABLE_ANTI_MULTIPLE_FARM
	P2P_MANAGER::instance().Logout(p->szName, p->bAFisWarping);
#else
	P2P_MANAGER::instance().Logout(p->szName);
#endif
}

int CInputP2P::Relay(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGRelay * p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE *) (c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		if (pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			// 귓속말 거부 상태에서 귓속말 거부.
			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;
		// bType 상위 4비트: Empire 번호
		// bType 하위 4비트: EWhisperType
		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if(p2->bType == 0x0F) {
			// 시스템 메세지 귓속말은 bType의 상위비트까지 모두 사용함.
			p2->bType = WHISPER_TYPE_SYSTEM;
		} else {
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf + sizeof(TPacketGCWhisper),
							p2->wSize - sizeof(TPacketGCWhisper),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

#ifdef ENABLE_FULL_NOTICE
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes, bool bBigFont)
#else
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes)
#endif
{
	TPacketGGNotice * p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
#ifdef ENABLE_FULL_NOTICE
	SendNotice(szBuf, bBigFont);
#else
	SendNotice(szBuf);
#endif
	return (p->lSize);
}

#ifdef __WORLD_BOSS_YUMA__
int CInputP2P::NewNotice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGNewNotice* p = (TPacketGGNewNotice*)c_pData;

	if (uiBytes < sizeof(TPacketGGNewNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNewNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendNewNotice(szBuf, p->szName, p->iSecondsToSpawn);

	return (p->lSize);
}
#endif

int CInputP2P::MonarchNotice(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGMonarchNotice * p = (TPacketGGMonarchNotice *) c_pData;

	if (uiBytes < p->lSize + sizeof(TPacketGGMonarchNotice))
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGMonarchNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendMonarchNotice(p->bEmpire, szBuf);
	return (p->lSize);
}

int CInputP2P::MonarchTransfer(LPDESC d, const char* c_pData)
{
	TPacketMonarchGGTransfer* p = (TPacketMonarchGGTransfer*) c_pData;
	LPCHARACTER pTargetChar = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (pTargetChar != NULL)
	{
		unsigned int qIndex = quest::CQuestManager::instance().GetQuestIndexByName("monarch_transfer");

		if (qIndex != 0)
		{
			pTargetChar->SetQuestFlag("monarch_transfer.x", p->x);
			pTargetChar->SetQuestFlag("monarch_transfer.y", p->y);
			quest::CQuestManager::instance().Letter(pTargetChar->GetPlayerID(), qIndex, 0);
		}
	}

	return 0;
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild * p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GUILD_SUBHEADER_GG_CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat))
					return -1;

				TPacketGGGuildChat * p = (TPacketGGGuildChat *) c_pData;

				if (g)
					g->P2PChat(p->szText);

				return sizeof(TPacketGGGuildChat);
			}

		case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int))
					return -1;

				int iBonus = *((int *) c_pData);
				CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int);
			}
#ifdef ENABLE_GUILD_REQUEST
	case GUILD_SUBHEADER_UPDATE_REQUEST:
	{
		CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
		if (pGuild)
			pGuild->ReloadRequest();
	}
	break;
#endif			
		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}
	return 0;
}

struct FuncShout
{
	const char * m_str;
	BYTE m_bEmpire;

	FuncShout(const char * str, BYTE bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}

	void operator () (LPDESC d)
	{
#ifdef ENABLE_NEWSTUFF
		if (!d->GetCharacter() || (!g_bGlobalShoutEnable && d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
#else
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
#endif
		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char * szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::Shout(const char * c_pData)
{
	TPacketGGShout * p = (TPacketGGShout *) c_pData;
	SendShout(p->szText, p->bEmpire);
}

void CInputP2P::Disconnect(const char * c_pData)
{
	TPacketGGDisconnect * p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char * c_pData)
{
	TPacketGGSetup * p = (TPacketGGSetup *) c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

#ifdef CROSS_CHANNEL_FRIEND_REQUEST
void CInputP2P::MessengerRequestAdd(const char* c_pData)
{
	TPacketGGMessengerRequest* p = (TPacketGGMessengerRequest*)c_pData;
	sys_log(0, "P2P: Messenger: Friend Request from %s to %s", p->account, p->target);

	LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindPC(p->target);
	MessengerManager::Instance().P2PRequestToAdd_Stage2(p->account, tch);
}
#endif

void CInputP2P::MessengerAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

#ifdef ENABLE_MESSENGER_BLOCK
void CInputP2P::MessengerBlockAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	MessengerManager::instance().__AddToBlockList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerBlockRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	MessengerManager::instance().__RemoveFromBlockList(p->szAccount, p->szCompanion);
}
#endif

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);
	if (ch && ch->GetMapIndex() < 10000)
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
		pw.mapIndex = ch->GetMapIndex();
		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch)
	{
		ch->WarpSet(p->x, p->y, p->mapIndex);
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex * p = (TPacketGGGuildWarMapIndex*) c_pData;
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm.FindGuild(p->dwGuildID1);
	CGuild * g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char * c_pData)
{
	TPacketGGTransfer * p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::XmasWarpSanta(const char * c_pData)
{
	TPacketGGXmasWarpSanta * p =(TPacketGGXmasWarpSanta *) c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int	iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay); // 50분있다가 새로운 산타가 나타남 (한국은 20분)

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.bHeader = HEADER_GG_XMAS_WARP_SANTA_REPLY;
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*) c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end()) {
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char * c_pData)
{
	TPacketGGLoginPing * p = (TPacketGGLoginPing *) c_pData;

	SendBillingExpire(p->szLogin, BILLING_DAY, 0, NULL);

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char * c_pData)
{
	TPacketGGBlockChat * p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}
// END_OF_BLOCK_CHAT
//

void CInputP2P::PCBangUpdate(const char* c_pData)
{
	TPacketPCBangUpdate* p = (TPacketPCBangUpdate*)c_pData;

	CPCBangManager::instance().RequestUpdateIPList(p->ulPCBangID);
}

void CInputP2P::IamAwake(LPDESC d, const char * c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

#ifdef ENABLE_DECORUM
extern bool g_bDecorumMaster;
int CInputP2P::DecorumArenaStart(LPDESC d, const char * c_pData, size_t uiBytes)
{		
	TPacketGGDecorumArenaStart * p = (TPacketGGDecorumArenaStart *) c_pData;
	
	if (uiBytes < p->lSize + sizeof(TPacketGGDecorumArenaStart))
		return -1;
		

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}
	
	if (!g_bDecorumMaster)
		return (p->lSize);
		
	
	c_pData += sizeof(TPacketGGDecorumArenaStart);
	DWORD * adwPids = (DWORD *) c_pData;
	
	set_ArenaTeam teamsA;
	set_ArenaTeam teamsB;
	for (int i = 0; i < p->nCountA + p->nCountB; i++)
	{
		DWORD pid = adwPids[i];
		if (i < p->nCountA )
			teamsA.insert(pid);
		else
		teamsB.insert(pid);
	}

	CDecoredArena * pkArena = CDecoredArenaManager::instance().CreateArena(p->dwMapIndex, p->bType);
	if (!pkArena)
		return (p->lSize);
		
	if (!CDecoredArenaManager::instance().AddTeams(pkArena->GetArenaMapIndex(), teamsA, teamsB))
	{
		pkArena->SetForceWarpOut();
		return (p->lSize);
	}
	
	int bSetPoints = CDecoredArenaManager::instance().GetTeamMemberByType(p->bType);
	if (!CDecoredArenaManager::instance().StartArena(pkArena->GetArenaMapIndex(), bSetPoints))
		pkArena->SetForceWarpOut();
		
	return (p->lSize);
}

int CInputP2P::DecorumRandomRequest(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGDecorumArenaRequest * p = (TPacketGGDecorumArenaRequest *) c_pData;
	
	if (uiBytes < p->lSize + sizeof(TPacketGGDecorumArenaRequest))
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}
	
	if (!g_bDecorumMaster){
		return (p->lSize);	
	}
		
	
	sys_log(0, "[Random Decorum Arena] DecorumRandomRequest type %d, dwMiddleLevel %d, wMiddleELO %d, dwOwnerPID %d", 
		p->bType, p->dwMiddleLevel, p->wMiddleELO, p->dwOwnerPID);
		
	int nLobbyID = CDecoredArenaManager::instance().ProcessRequestArena(p->dwOwnerPID, p->bType, p->wMiddleELO, p->dwMiddleLevel);
	
	if (nLobbyID == 0){
		return (p->lSize);
	}
	
	TArenaLobby * pkLobby = CDecoredArenaManager::instance().GetLobby(nLobbyID);
	pkLobby->bIsForRandomArena = true;
		
	if (p->nCount == 1){
		CDecoredArenaManager::instance().Apply(p->dwOwnerPID, nLobbyID);
	}
	else
	{
		c_pData += sizeof(TPacketGGDecorumArenaRequest);
		DWORD * adwPids = (DWORD *) c_pData;
		
		for (BYTE i = 0; i < p->nCount; i++)
		{
			if (p->nCountA != 0 && i < p->nCountA){
				CDecoredArenaManager::instance().Apply(adwPids[i], nLobbyID, "", 1);
			}
			else if (p->nCountB != 0 && i < p->nCountB){
				CDecoredArenaManager::instance().Apply(adwPids[i], nLobbyID, "", 2);
			}
			else{
				CDecoredArenaManager::instance().Apply(adwPids[i], nLobbyID);
			}
				
		}
	}
		
	return (p->lSize);
}

void CInputP2P::DecorumRandomDelete(LPDESC d, const char * c_pData)
{
	TPacketGGDecorumArenaDelete * p = (TPacketGGDecorumArenaDelete *) c_pData;
	sys_log(0, "[Random Decorum Arena] DecorumRandomDelete dwPID %d", p->dwPID);
	
	CDecoredArenaManager::instance().DeleteRequestArena(p->dwPID, false);
}

void CInputP2P::DecorumRandomBroadcast(LPDESC d, const char * c_pData)
{
	TPacketGGDecorumArenaBroadcast * p = (TPacketGGDecorumArenaBroadcast *) c_pData;
	if (p->PacketType == DECORUM_RANDOM_ARENA_BROADCAST_CG && g_bDecorumMaster)
	{
		sys_log(0, "[Random Decorum Arena] DecorumRandomBroadcast from client to server. ApplicantsID = %d in ArenaID %d", p->dwArgument, p->dwArenaID);
		CDecoredArenaManager::instance().Apply(p->dwArgument, p->dwArenaID);
		CDecoredArenaManager::instance().CheckRandomLobby(p->dwArenaID);
	}
	else if (p->PacketType == DECORUM_RANDOM_ARENA_BROADCAST_GC && !g_bDecorumMaster)
	{
		sys_log(0, "[Random Decorum Arena] DecorumRandomBroadcast from server to server");
		CDecorumManager::instance().BroadcastRandomLobby(p->dwArenaID, p->bArenaType, p->dwMiddleLevel, p->dwArgument, p->dwExceptPID);
	}
}

void CInputP2P::DecorumArenaWarpIn(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch)
	{
		if(ch->GetHorse())
			ch->HorseSummon(false);
		ch->WarpSet(p->x, p->y, p->mapIndex);
	}
}
#endif

#ifdef OFFLINE_SHOP
void CInputP2P::SendOfflineShopMessage(LPDESC d, const char * c_pData)
{
	TPacketGGOfflineShopMessage * p = (TPacketGGOfflineShopMessage *)c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (ch)
	{
		LPDESC pkVictimDesc = ch->GetDesc();

		if (pkVictimDesc)
		{
			char msg[CHAT_MAX_LEN + 1];
			snprintf(msg, sizeof(msg), LC_TEXT("Itemul tau %s a fost batut, cumparatorul este: %s."), p->szItemName, p->szName);

			TPacketGCWhisper pack;

			int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

			pack.bHeader = HEADER_GC_WHISPER;
			pack.wSize = sizeof(TPacketGCWhisper) + len;
			pack.bType = WHISPER_TYPE_SYSTEM;
			// pack.bLevel = 0;
			strlcpy(pack.szNameFrom, "[Market Assistant]", sizeof(pack.szNameFrom));

			TEMP_BUFFER buf;

			buf.write(&pack, sizeof(TPacketGCWhisper));
			buf.write(msg, len);

			pkVictimDesc->Packet(buf.read_peek(), buf.size());
		}
	}
}
#endif

#ifdef ENABLE_MAINTENANCE_SYSTEM
void CInputP2P::RecvShutdown(LPDESC d, const char * c_pData)
{
	TPacketGGShutdown* p = (TPacketGGShutdown*) c_pData;

	if (p->iShutdownTimer < 0)
	{
		__StopCurrentShutdown();
	}
	else
	{
		sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
		
		__StartNewShutdown(p->iShutdownTimer, p->bMaintenance, p->iMaintenanceDuration);
	}
}
int CInputP2P::PlayerPacket(const char * c_pData, size_t uiBytes)
{
	TPacketGGPlayerPacket* p = (TPacketGGPlayerPacket*) c_pData;

	if (uiBytes < sizeof(TPacketGGPlayerPacket) + p->size)
		return -1;
	c_pData += sizeof(TPacketGGPlayerPacket);

	if (p->size <= 0)
		return -1;

	const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
	for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
	{
		if (it->second->GetDesc()->IsPhase(PHASE_GAME) || it->second->GetDesc()->IsPhase(PHASE_DEAD))
		{
			// if (p->language == -1 || p->language == it->second->GetLanguageID())
				it->second->GetDesc()->Packet(c_pData, p->size);
		}
	}

	return p->size;
}
#endif


int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", bHeader);


	int iExtraLen = 0;

#ifdef HANDSHAKE_FIX
	// Auth server is not allowed for p2p
	if (g_bAuthServer)
	{
		// Clearing buffers for dynamic packets
		switch (bHeader)
		{
		case HEADER_GG_RELAY:
		{
			TPacketGGRelay * p = (TPacketGGRelay *)c_pData;
			if (m_iBufferLeft < sizeof(TPacketGGRelay) + p->lSize)
				iExtraLen = -1;
			else
				iExtraLen = p->lSize;
		}
		break;
		case HEADER_GG_NOTICE:
		{
			TPacketGGNotice * p = (TPacketGGNotice *)c_pData;
			if (m_iBufferLeft < sizeof(TPacketGGNotice) + p->lSize)
				iExtraLen = -1;
			else
				iExtraLen = p->lSize;
		}
		break;
		case HEADER_GG_GUILD:
		{
			iExtraLen = m_iBufferLeft - sizeof(TPacketGGGuild);
		}
		break;
		case HEADER_GG_MONARCH_NOTICE:
		{
			TPacketGGMonarchNotice * p = (TPacketGGMonarchNotice *)c_pData;
			if (m_iBufferLeft < p->lSize + sizeof(TPacketGGMonarchNotice))
				iExtraLen = -1;
			else
				iExtraLen = p->lSize;
		}
		break;
		}

		return iExtraLen;
	}
#endif

	switch (bHeader)
	{
		case HEADER_GG_SETUP:
			Setup(d, c_pData);
			break;

		case HEADER_GG_LOGIN:
			Login(d, c_pData);
			break;

		case HEADER_GG_LOGOUT:
			Logout(d, c_pData);
			break;

#ifdef HANDSHAKE_FIX
		case HEADER_GG_HANDSHAKE_VALIDATION:
			DESC_MANAGER::instance().AddToHandshakeWhiteList((const TPacketGGHandshakeValidate *)c_pData);
			break;
#endif

		case HEADER_GG_RELAY:
			if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#ifdef ENABLE_FULL_NOTICE
		case HEADER_GG_BIG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft, true)) < 0)
				return -1;
			break;
#endif
		case HEADER_GG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

#ifdef __WORLD_BOSS_YUMA__
		case HEADER_GG_NEW_NOTICE:
			if ((iExtraLen = NewNotice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif
		case HEADER_GG_SHUTDOWN:
#ifdef ENABLE_MAINTENANCE_SYSTEM
			RecvShutdown(d, c_pData);
#else
			sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
			Shutdown(10);
#endif
			break;
#ifdef ENABLE_MAINTENANCE_SYSTEM
		case HEADER_GG_PLAYER_PACKET:
			if ((iExtraLen = PlayerPacket(c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif

		case HEADER_GG_GUILD:
			if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHOUT:
			Shout(c_pData);
			break;

		case HEADER_GG_DISCONNECT:
			Disconnect(c_pData);
			break;

		case HEADER_GG_MESSENGER_ADD:
			MessengerAdd(c_pData);
			break;

#ifdef CROSS_CHANNEL_FRIEND_REQUEST
	case HEADER_GG_MESSENGER_REQUEST_ADD:
		MessengerRequestAdd(c_pData);
		break;
#endif

		case HEADER_GG_MESSENGER_REMOVE:
			MessengerRemove(c_pData);
			break;
			
		#ifdef ENABLE_MESSENGER_BLOCK
		case HEADER_GG_MESSENGER_BLOCK_ADD:
			MessengerBlockAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_BLOCK_REMOVE:
			MessengerBlockRemove(c_pData);
			break;
		#endif

		case HEADER_GG_FIND_POSITION:
			FindPosition(d, c_pData);
			break;

		case HEADER_GG_WARP_CHARACTER:
			WarpCharacter(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(c_pData);
			break;

		case HEADER_GG_TRANSFER:
			Transfer(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA:
			XmasWarpSanta(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA_REPLY:
			XmasWarpSantaReply(c_pData);
			break;

		case HEADER_GG_RELOAD_CRC_LIST:
			LoadValidCRCList();
			break;

		case HEADER_GG_CHECK_CLIENT_VERSION:
			CheckClientVersion();
			break;

		case HEADER_GG_LOGIN_PING:
			LoginPing(d, c_pData);
			break;

		case HEADER_GG_BLOCK_CHAT:
			BlockChat(c_pData);
			break;

		case HEADER_GG_SIEGE:
			{
				TPacketGGSiege* pSiege = (TPacketGGSiege*)c_pData;
				castle_siege(pSiege->bEmpire, pSiege->bTowerCount);
			}
			break;

		case HEADER_GG_MONARCH_NOTICE:
			if ((iExtraLen = MonarchNotice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_MONARCH_TRANSFER :
			MonarchTransfer(d, c_pData);
			break;

		case HEADER_GG_PCBANG_UPDATE :
			PCBangUpdate(c_pData);
			break;

		case HEADER_GG_CHECK_AWAKENESS:
			IamAwake(d, c_pData);
			break;
#ifdef ENABLE_DECORUM
		case HEADER_GG_DECORUM_ARENA_START:
			if ((iExtraLen = DecorumArenaStart(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
			
		case HEADER_GG_DECORUM_RANDOM_REQUEST:
			if ((iExtraLen = DecorumRandomRequest(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
			
		case HEADER_GG_DECORUM_RANDOM_DELETE:
			DecorumRandomDelete(d, c_pData);
			break;
			
		case HEADER_GG_DECORUM_RANDOM_BROADCAST:
			DecorumRandomBroadcast(d, c_pData);
			break;
		case HEADER_GG_WARP_CHARACTER_ARENA:
			DecorumArenaWarpIn(c_pData);
			break;
#endif			
#ifdef OFFLINE_SHOP
		case HEADER_GG_OFFLINE_SHOP_SEND_MESSAGE:
			SendOfflineShopMessage(d, c_pData);
			break;
#endif
#ifdef ENABLE_SWITCHBOT
		case HEADER_GG_SWITCHBOT:
			Switchbot(d, c_pData);
			break;
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
		case HEADER_GG_ANTI_FARM:
			RecvAntiFarmUpdateStatus(d, c_pData);
			break;
#endif
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
void CInputP2P::Switchbot(LPDESC d, const char* c_pData)
{
	const TPacketGGSwitchbot* p = reinterpret_cast<const TPacketGGSwitchbot*>(c_pData);
	if (p->wPort != mother_port)
	{
		return;
	}

	CSwitchbotManager::Instance().P2PReceiveSwitchbot(p->table);
}
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
auto CInputP2P::RecvAntiFarmUpdateStatus(LPDESC d, const char* c_pData) -> void
{
	if (!d) return;

	CAntiMultipleFarm::TP2PChangeDropStatus* p = (CAntiMultipleFarm::TP2PChangeDropStatus*)c_pData;

	std::vector <DWORD> dwPIDs;
	for (uint8_t i = 0; i < MULTIPLE_FARM_MAX_ACCOUNT; ++i)
		dwPIDs.emplace_back(p->dwPIDs[i]);

	CAntiMultipleFarm::instance().P2PSendBlockDropStatusChange(p->cMAIf, dwPIDs);
}
#endif
