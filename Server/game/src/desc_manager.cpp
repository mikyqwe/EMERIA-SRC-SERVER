#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "desc.h"
#include "desc_p2p.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "messenger_manager.h"
#include "p2p.h"
#include "ip_ban.h"
#include "dev_log.h"
#include "ClientPackageCryptInfo.h"
#ifdef OFFLINE_SHOP
#include "questmanager.h"
#endif
#include "EventFunctionHandler.h"
#include "event.h"

#include <boost/container/map.hpp>

struct valid_ip
{
	const char *	ip;
	BYTE	network;
	BYTE	mask;
};

static struct valid_ip admin_ip[] =
{
	{ "210.123.10",     128,    128     },
	{ "\n",             0,      0       }
};

int IsValidIP(struct valid_ip* ip_table, const char *host)
{
	int         i, j;
	char        ip_addr[256];

	for (i = 0; *(ip_table + i)->ip != '\n'; ++i)
	{
		j = 255 - (ip_table + i)->mask;

		do
		{
			snprintf(ip_addr, sizeof(ip_addr), "%s.%d", (ip_table + i)->ip, (ip_table + i)->network + j);

			if (!strcmp(ip_addr, host))
				return TRUE;

			if (!j)
				break;
		}
		while (j--);
	}

	return FALSE;
}

#if defined(__IMPROVED_HANDSHAKE_PROCESS__)
/*
* Title: Improved Handshake Process
* Description: An Anti-spam solution that limits the amount
*              of handshakes per second and per attempts.
* Date: YMD.2021.09.07
* Author: Owsap, OSP (Owsap Server Protection)
*
* Copyright 2021 Owsap Productions
*/
void DESC_MANAGER::AcceptHandshake(const char* c_szHost, DWORD dwHandshakeTime)
{
	// Find if the host is in the AcceptHostHandshakeVector
	AcceptHostHandshakeVector::iterator it = std::find_if(m_vecAcceptHostHandshake.begin(), m_vecAcceptHostHandshake.end(),
		[&c_szHost](const PairedStringDWORD& element)
		{ return (element.first == c_szHost); }
	);
	// Check if host already exists in the AcceptHostHandshakeVector
	if (it != m_vecAcceptHostHandshake.end())
	{
		// Check if the cores pulse is greater than the last handshake time set @ DESC::Setup
		if (thecore_pulse() > it->second)
			m_vecAcceptHostHandshake.erase(it);
	}
	else // Set host handshake time
		m_vecAcceptHostHandshake.emplace_back(std::make_pair(c_szHost, dwHandshakeTime));
}

bool DESC_MANAGER::IsIntrusiveHandshake(const char* c_szHost)
{
	if (!m_vecAcceptHostHandshake.empty())
	{
		// Find if the host is in the AcceptHostHandshakeVector and if the cores
		// pulse is greater than the last handshake time set @ DESC::Setup
		AcceptHostHandshakeVector::iterator it = std::find_if(m_vecAcceptHostHandshake.begin(), m_vecAcceptHostHandshake.end(),
			[&c_szHost](const PairedStringDWORD& element)
			{ return (element.first == c_szHost && element.second > thecore_pulse()); }
		);
		if (it != m_vecAcceptHostHandshake.end())
			return true;
	}
	return false;
}

void DESC_MANAGER::SetIntrusiveCount(const char* c_szHost, bool bReset)
{
	// Find if the host is in the IntrusiveHostCountVector
	IntrusiveHostCountVector::iterator it = std::find_if(m_vecIntrusiveHostCount.begin(), m_vecIntrusiveHostCount.end(),
		[&c_szHost](const PairedStringDWORD& element)
		{ return (element.first == c_szHost); }
	);
	// Set intrusive host count if exists in the IntrusiveHostCountVector
	if (it != m_vecIntrusiveHostCount.end())
	{
		if (bReset)
			it->second = 0;
		else
			it->second = it->second + 1;
	}
	else
		// Emplace the host into the IntrusiveHostCountVector with count set to 1
		m_vecIntrusiveHostCount.emplace_back(std::make_pair(c_szHost, 1));
}

int DESC_MANAGER::GetIntrusiveCount(const char* c_szHost)
{
	if (!m_vecIntrusiveHostCount.empty())
	{
		// Find if the host is in the IntrusiveHostCountVector
		IntrusiveHostCountVector::iterator it = std::find_if(m_vecIntrusiveHostCount.begin(), m_vecIntrusiveHostCount.end(),
			[&c_szHost](const PairedStringDWORD& element)
			{ return (element.first == c_szHost); }
		);
		// Return the intrusive host count
		if (it != m_vecIntrusiveHostCount.end())
			return it->second;
	}
	return 0;
}

void DESC_MANAGER::SetIntruder(const char* c_szHost, DWORD dwDelayHandshakeTime)
{
	// Reset intrusive host count
	SetIntrusiveCount(c_szHost, true /* reset */);

	// Find if the host is in the IntruderHostVector
	IntruderHostVector::iterator it = std::find_if(m_vecIntruderHost.begin(), m_vecIntruderHost.end(),
		[&c_szHost](const PairedStringDWORD& element)
		{ return (element.first == c_szHost); }
	);
	// Set intruder host with the next (delayed) handshake time
	if (it != m_vecIntruderHost.end())
		it->second = dwDelayHandshakeTime;
	else
		// Emplace the host into the IntruderHostVector with the next (delayed) handshake time
		m_vecIntruderHost.emplace_back(std::make_pair(c_szHost, dwDelayHandshakeTime));
}

bool DESC_MANAGER::IsIntruder(const char* c_szHost)
{
	if (!m_vecIntruderHost.empty())
	{
		// Find if the host is in the IntruderHostVector and if the cores pulse
		// is less than the next (delayed) handshake time
		IntruderHostVector::iterator it = std::find_if(m_vecIntruderHost.begin(), m_vecIntruderHost.end(),
			[&c_szHost](const PairedStringDWORD& element)
			{ return (element.first == c_szHost && element.second > thecore_pulse()); }
		);
		if (it != m_vecIntruderHost.end())
			return true;
	}
	return false;
}

void DESC_MANAGER::AllowHandshake(const char* c_szHost)
{
	// Clear the intrusive host count
	if (!m_vecIntrusiveHostCount.empty())
	{
		IntrusiveHostCountVector::iterator it = std::find_if(m_vecIntrusiveHostCount.begin(), m_vecIntrusiveHostCount.end(),
			[&c_szHost](const PairedStringDWORD& element)
			{ return (element.first == c_szHost); }
		);
		if (it != m_vecIntrusiveHostCount.end())
			m_vecIntrusiveHostCount.erase(it);
	}

	// Clear the intruder host
	if (!m_vecIntruderHost.empty())
	{
		IntruderHostVector::iterator it = std::find_if(m_vecIntruderHost.begin(), m_vecIntruderHost.end(),
			[&c_szHost](const PairedStringDWORD& element)
			{ return (element.first == c_szHost); }
		);
		if (it != m_vecIntruderHost.end())
			m_vecIntruderHost.erase(it);
	}
}
#endif

DESC_MANAGER::DESC_MANAGER() : m_bDestroyed(false)
{
	Initialize();
	//NOTE : Destroy 끝에서 Initialize 를 부르는건 또 무슨 짓이냐..-_-; 정말

	m_pPackageCrypt = new CClientPackageCryptInfo;
}

DESC_MANAGER::~DESC_MANAGER()
{
	Destroy();
	delete m_pPackageCrypt;
}

void DESC_MANAGER::Initialize()
{
	m_iSocketsConnected = 0;
	m_iHandleCount = 0;
	m_iLocalUserCount = 0;
	memset(m_aiEmpireUserCount, 0, sizeof(m_aiEmpireUserCount));
	m_bDisconnectInvalidCRC = false;
#if defined(__IMPROVED_HANDSHAKE_PROCESS__)
	m_vecAcceptHostHandshake.clear();
	m_vecIntrusiveHostCount.clear();
	m_vecIntruderHost.clear();
#endif
}

void DESC_MANAGER::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	DESC_SET::iterator i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->GetType() == DESC_TYPE_CONNECTOR)
			continue;

		if (d->IsPhase(PHASE_P2P))
			continue;

		DestroyDesc(d, false);
		m_set_pkDesc.erase(ci);
	}

	i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		DestroyDesc(d, false);
		m_set_pkDesc.erase(ci);
	}

	m_set_pkClientDesc.clear();

	//m_AccountIDMap.clear();
	m_map_loginName.clear();
	m_map_handle.clear();

	Initialize();
}

DWORD DESC_MANAGER::CreateHandshake()
{
	char crc_buf[8];
	crc_t crc;
	DESC_HANDSHAKE_MAP::iterator it;

RETRY:
	do
	{
		DWORD val = thecore_random() % (1024 * 1024);

		*(DWORD *) (crc_buf    ) = val;
		*((DWORD *) crc_buf + 1) = get_global_time();

		crc = GetCRC32(crc_buf, 8);
		it = m_map_handshake.find(crc);
	}
	while (it != m_map_handshake.end());

	if (crc == 0)
		goto RETRY;

	return (crc);
}

LPDESC DESC_MANAGER::AcceptDesc(LPFDWATCH fdw, socket_t s)
{
	socket_t                    desc;
	LPDESC						newd;
	static struct sockaddr_in   peer;
	static char					host[MAX_HOST_LENGTH + 1];

	if ((desc = socket_accept(s, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	if (g_bAuthServer)
	{
		if (IsBanIP(peer.sin_addr))
		{
			sys_log(0, "connection from %s was banned.", host);
			socket_close(desc);
			return NULL;
		}
	}

	if (!IsValidIP(admin_ip, host)) // admin_ip 에 등록된 IP 는 최대 사용자 수에 구애받지 않는다.
	{
		if (m_iSocketsConnected >= MAX_ALLOW_USER)
		{
			sys_err("max connection reached. MAX_ALLOW_USER = %d", MAX_ALLOW_USER);
			socket_close(desc);
			return NULL;
		}
	}


#ifdef HANDSHAKE_FIX
	// If it's not an auth server - check for validation first
	if (!g_bAuthServer)
	{
		if (!IsOnHandshakeWhitelist(peer))
		{
			// sys_log(0, "Host %s has not validated through login!", host);
			socket_close(desc);
			return NULL;
		}
	}

	// Let's check if a handshake from this host is already ongoing
	if (GetHostHandshake(peer))
	{
		sys_log(0, "Handshake from %s is not permitted!", host);
		socket_close(desc);
		return NULL;
	}


	static const int HOST_CONNECTION_LIMIT = 3;
	// In case if host completed handshake process let's check if it doesn't reach the limit
	if (GetHostConnectionCount(peer) >= HOST_CONNECTION_LIMIT)
	{
		sys_log(0, "Host %s connection limit has been reached!", host);
		socket_close(desc);
		return NULL;
	}

	// And block intrusive connections as well
	if (IsIntrusiveConnection(host))
	{
		sys_log(0, "Host %s is intrusive!", host);
		socket_close(desc);
		return NULL;
	}
#endif

#if defined(__IMPROVED_HANDSHAKE_PROCESS__)
	// Check if the host is an intruder
	if (IsIntruder(host))
	{
		if (INTRUSIVE_HANDSHAKE_LOG)
			sys_log(0, "intrusive connection from %s was blocked temporarily.", host);
		socket_close(desc);
		return NULL;
	}

	// Check if the handshake is intrusive
	if (IsIntrusiveHandshake(host))
	{
		// Get the intrusive count
		int iIntrusiveCount = GetIntrusiveCount(host);
		if (iIntrusiveCount >= INTRUSIVE_HANDSHAKE_LIMIT)
		{
			// Set intruder with the next (delayed) handshake time
			SetIntruder(host, thecore_pulse() + PASSES_PER_SEC(INTRUSIVE_HANDSHAKE_NEXT_PULSE));
			socket_close(desc);
			return NULL;
		}

		// Set intrusive count
		if (INTRUSIVE_HANDSHAKE_LOG)
			sys_log(0, "intrusive connection from %s. %d/%d", host, iIntrusiveCount, INTRUSIVE_HANDSHAKE_LIMIT);
		SetIntrusiveCount(host, false /* reset */);
	}
	else
		// Allow handshake and clear previous intrusive tracking
		AllowHandshake(host);
#endif

	newd = M2_NEW DESC;

	crc_t handshake = CreateHandshake();

	if (!newd->Setup(fdw, desc, peer, ++m_iHandleCount, handshake))
	{
		socket_close(desc);
		M2_DELETE(newd);
		return NULL;
	}

	m_map_handshake.insert(DESC_HANDSHAKE_MAP::value_type(handshake, newd));
	m_map_handle.insert(DESC_HANDLE_MAP::value_type(newd->GetHandle(), newd));

	m_set_pkDesc.insert(newd);
	++m_iSocketsConnected;
	return (newd);
}

LPDESC DESC_MANAGER::AcceptP2PDesc(LPFDWATCH fdw, socket_t bind_fd)
{
	socket_t           fd;
	struct sockaddr_in peer;
	char               host[MAX_HOST_LENGTH + 1];

	if ((fd = socket_accept(bind_fd, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	LPDESC_P2P pkDesc = M2_NEW DESC_P2P;

	if (!pkDesc->Setup(fdw, fd, host, peer.sin_port))
	{
		sys_err("DESC_MANAGER::AcceptP2PDesc : Setup failed");
		socket_close(fd);
		M2_DELETE(pkDesc);
		return NULL;
	}

	m_set_pkDesc.insert(pkDesc);
	++m_iSocketsConnected;

	sys_log(0, "DESC_MANAGER::AcceptP2PDesc  %s:%u", host, peer.sin_port);
	P2P_MANAGER::instance().RegisterAcceptor(pkDesc);
	return (pkDesc);
}

void DESC_MANAGER::ConnectAccount(const std::string& login, LPDESC d)
{
dev_log(LOG_DEB0, "BBBB ConnectAccount(%s)", login.c_str());
	m_map_loginName.insert(DESC_LOGINNAME_MAP::value_type(login,d));
}

void DESC_MANAGER::DisconnectAccount(const std::string& login)
{
dev_log(LOG_DEB0, "BBBB DisConnectAccount(%s)", login.c_str());
	m_map_loginName.erase(login);
}

void DESC_MANAGER::DestroyDesc(LPDESC d, bool bEraseFromSet)
{
	if (bEraseFromSet)
		m_set_pkDesc.erase(d);

	if (d->GetHandshake())
		m_map_handshake.erase(d->GetHandshake());

	if (d->GetHandle() != 0)
		m_map_handle.erase(d->GetHandle());
	else
		m_set_pkClientDesc.erase((LPCLIENT_DESC) d);

	// Explicit call to the virtual function Destroy()
	d->Destroy();

	M2_DELETE(d);
	--m_iSocketsConnected;
}

void DESC_MANAGER::DestroyClosed()
{
	DESC_SET::iterator i = m_set_pkDesc.begin();

	while (i != m_set_pkDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->IsPhase(PHASE_CLOSE))
		{
			if (d->GetType() == DESC_TYPE_CONNECTOR)
			{
				LPCLIENT_DESC client_desc = (LPCLIENT_DESC)d;

				if (client_desc->IsRetryWhenClosed())
				{
					client_desc->Reset();
					continue;
				}
			}

			DestroyDesc(d, false);
			m_set_pkDesc.erase(ci);
		}
	}
}

LPDESC DESC_MANAGER::FindByLoginName(const std::string& login)
{
	DESC_LOGINNAME_MAP::iterator it = m_map_loginName.find(login);

	if (m_map_loginName.end() == it)
		return NULL;

	return (it->second);
}

LPDESC DESC_MANAGER::FindByHandle(DWORD handle)
{
	DESC_HANDLE_MAP::iterator it = m_map_handle.find(handle);

	if (m_map_handle.end() == it)
		return NULL;

	return (it->second);
}

const DESC_MANAGER::DESC_SET & DESC_MANAGER::GetClientSet()
{
	return m_set_pkDesc;
}

struct name_with_desc_func
{
	const char * m_name;

	name_with_desc_func(const char * name) : m_name(name)
	{
	}

	bool operator () (LPDESC d)
	{
		if (d->GetCharacter() && !strcmp(d->GetCharacter()->GetName(), m_name))
			return true;

		return false;
	}
};

LPDESC DESC_MANAGER::FindByCharacterName(const char *name)
{
	DESC_SET::iterator it = std::find_if (m_set_pkDesc.begin(), m_set_pkDesc.end(), name_with_desc_func(name));
	return (it == m_set_pkDesc.end()) ? NULL : (*it);
}

LPCLIENT_DESC DESC_MANAGER::CreateConnectionDesc(LPFDWATCH fdw, const char * host, WORD port, int iPhaseWhenSucceed, bool bRetryWhenClosed)
{
	LPCLIENT_DESC newd;

	newd = M2_NEW CLIENT_DESC;

	newd->Setup(fdw, host, port);
	newd->Connect(iPhaseWhenSucceed);
	newd->SetRetryWhenClosed(bRetryWhenClosed);

	m_set_pkDesc.insert(newd);
	m_set_pkClientDesc.insert(newd);

	++m_iSocketsConnected;
	return (newd);
}

struct FuncTryConnect
{
	void operator () (LPDESC d)
	{
		((LPCLIENT_DESC)d)->Connect();
	}
};

void DESC_MANAGER::TryConnect()
{
	FuncTryConnect f;
	std::for_each(m_set_pkClientDesc.begin(), m_set_pkClientDesc.end(), f);
}

bool DESC_MANAGER::IsP2PDescExist(const char * szHost, WORD wPort)
{
	CLIENT_DESC_SET::iterator it = m_set_pkClientDesc.begin();

	while (it != m_set_pkClientDesc.end())
	{
		LPCLIENT_DESC d = *(it++);

		if (!strcmp(d->GetP2PHost(), szHost) && d->GetP2PPort() == wPort)
			return true;
	}

	return false;
}

LPDESC DESC_MANAGER::FindByHandshake(DWORD dwHandshake)
{
	DESC_HANDSHAKE_MAP::iterator it = m_map_handshake.find(dwHandshake);

	if (it == m_map_handshake.end())
		return NULL;

	return (it->second);
}

class FuncWho
{
	public:
		int iTotalCount;
		int aiEmpireUserCount[EMPIRE_MAX_NUM];

		FuncWho()
		{
			iTotalCount = 0;
			memset(aiEmpireUserCount, 0, sizeof(aiEmpireUserCount));
		}

		void operator() (LPDESC d)
		{
			if (d->GetCharacter())
			{
				++iTotalCount;
				++aiEmpireUserCount[d->GetEmpire()];
			}
		}
};

void DESC_MANAGER::UpdateLocalUserCount()
{
	const DESC_SET & c_ref_set = GetClientSet();
	FuncWho f;
	f = std::for_each(c_ref_set.begin(), c_ref_set.end(), f);

	m_iLocalUserCount = f.iTotalCount;
	thecore_memcpy(m_aiEmpireUserCount, f.aiEmpireUserCount, sizeof(m_aiEmpireUserCount));

	m_aiEmpireUserCount[1] += P2P_MANAGER::instance().GetEmpireUserCount(1);
	m_aiEmpireUserCount[2] += P2P_MANAGER::instance().GetEmpireUserCount(2);
	m_aiEmpireUserCount[3] += P2P_MANAGER::instance().GetEmpireUserCount(3);
}

void DESC_MANAGER::GetUserCount(int & iTotal, int ** paiEmpireUserCount, int & iLocalCount)
{
	*paiEmpireUserCount = &m_aiEmpireUserCount[0];

	int iCount = P2P_MANAGER::instance().GetCount();
	if (iCount < 0)
	{
		sys_err("P2P_MANAGER::instance().GetCount() == -1");
	}
	iTotal = m_iLocalUserCount + iCount;
	iLocalCount = m_iLocalUserCount;
}


DWORD DESC_MANAGER::MakeRandomKey(DWORD dwHandle)
{
	DWORD random_key = thecore_random();
	m_map_handle_random_key.insert(std::make_pair(dwHandle, random_key));
	return random_key;
}

bool DESC_MANAGER::GetRandomKey(DWORD dwHandle, DWORD* prandom_key)
{
	DESC_HANDLE_RANDOM_KEY_MAP::iterator it = m_map_handle_random_key.find(dwHandle);

	if (it == m_map_handle_random_key.end())
		return false;

	*prandom_key = it->second;
	return true;
}

LPDESC DESC_MANAGER::FindByLoginKey(DWORD dwKey)
{
	std::map<DWORD, CLoginKey *>::iterator it = m_map_pkLoginKey.find(dwKey);

	if (it == m_map_pkLoginKey.end())
		return NULL;

	return it->second->m_pkDesc;
}


DWORD DESC_MANAGER::CreateLoginKey(LPDESC d)
{
	DWORD dwKey = 0;

	do
	{
		dwKey = number(1, INT_MAX);

		if (m_map_pkLoginKey.find(dwKey) != m_map_pkLoginKey.end())
			continue;

		CLoginKey * pkKey = M2_NEW CLoginKey(dwKey, d);
		d->SetLoginKey(pkKey);
		m_map_pkLoginKey.insert(std::make_pair(dwKey, pkKey));
		break;
	} while (1);

	return dwKey;
}

void DESC_MANAGER::ProcessExpiredLoginKey()
{
	DWORD dwCurrentTime = get_dword_time();

	std::map<DWORD, CLoginKey *>::iterator it, it2;

	it = m_map_pkLoginKey.begin();

	while (it != m_map_pkLoginKey.end())
	{
		it2 = it++;

		if (it2->second->m_dwExpireTime == 0)
			continue;

		if (dwCurrentTime - it2->second->m_dwExpireTime > 60000)
		{
			M2_DELETE(it2->second);
			m_map_pkLoginKey.erase(it2);
		}
	}
}

bool DESC_MANAGER::LoadClientPackageCryptInfo(const char* pDirName)
{
	return m_pPackageCrypt->LoadPackageCryptInfo(pDirName);
}
#ifdef __FreeBSD__
void DESC_MANAGER::NotifyClientPackageFileChanged( const std::string& dirName, eFileUpdatedOptions eUpdateOption )
{
	 Instance().LoadClientPackageCryptInfo(dirName.c_str());
}
#endif


void DESC_MANAGER::SendClientPackageCryptKey( LPDESC desc )
{
	if( !desc )
	{
		return;
	}

	TPacketGCHybridCryptKeys packet;
	{
		packet.bHeader = HEADER_GC_HYBRIDCRYPT_KEYS;
		m_pPackageCrypt->GetPackageCryptKeys( &(packet.pDataKeyStream), packet.KeyStreamLen );
	}

	if( packet.KeyStreamLen > 0 )
	{
		if (test_server)
		{
			// keys를 string으로 남기는 건 문제가 있음. 중간에 NULL 있으면 잘릴테니.. 그래도 혹시 모르니 남김.
			sys_log(0, "[PackageCryptInfo] send to %s. (keys: %s, len: %d)", desc->GetAccountTable().login, std::string((char*)packet.pDataKeyStream).c_str(), packet.KeyStreamLen);
		}
		desc->Packet( packet.GetStreamData(), packet.GetStreamSize() );
	}
}

void DESC_MANAGER::SendClientPackageSDBToLoadMap( LPDESC desc, const char* pMapName )
{
	if( !desc )
	{
		return;
	}

	TPacketGCPackageSDB packet;
	{
		packet.bHeader      = HEADER_GC_HYBRIDCRYPT_SDB;
		if( !m_pPackageCrypt->GetRelatedMapSDBStreams( pMapName, &(packet.m_pDataSDBStream), packet.iStreamLen ) )
			return;
		if (test_server)
			sys_log(0, "[PackageCryptInfo] send to %s from map %s. (SDB len: %d)", desc->GetAccountTable().login, pMapName, packet.iStreamLen);
	}

	if( packet.iStreamLen > 0 )
	{
		desc->Packet( packet.GetStreamData(), packet.GetStreamSize());
	}
}

#ifdef OFFLINE_SHOP
struct online_count_packet_func
{
	const DWORD dwOnlinePlayers;
	const DWORD dwOnlineShops;

	online_count_packet_func(const DWORD dwOnlinePlayers, const DWORD dwOnlineShops) : dwOnlinePlayers(dwOnlinePlayers), dwOnlineShops(dwOnlineShops) {}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;
/* 
		d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "CurrentPlayerOnline %d",
			dwOnlinePlayers + quest::CQuestManager::instance().GetEventFlag("online_count"));

		d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "CurrentShopOnline %d",
			dwOnlineShops + quest::CQuestManager::instance().GetEventFlag("shop_count")); */
	}
};

void DESC_MANAGER::BroadcastOnlineCount(DWORD dwOnlinePlayers, DWORD dwOnlineShops)
{
	const DESC_SET & c_ref_set = GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), online_count_packet_func(dwOnlinePlayers, dwOnlineShops));
}
#endif

#ifdef HANDSHAKE_FIX
EVENTINFO(desc_manager_garbage_collector_info)
{
};

EVENTFUNC(desc_manager_garbage_collector_event)
{
	DESC_MANAGER::instance().ConnectionCollector();
	return PASSES_PER_SEC(1);
}


bool DESC_MANAGER::GetHostHandshake(const struct sockaddr_in & c_rSockAddr)
{
	for (const auto & rRec : m_map_handshake)
	{
		if (rRec.second->GetHostName() == inet_ntoa(c_rSockAddr.sin_addr))
		{
			if (rRec.second->IsPhase(PHASE_HANDSHAKE))
				return true;
		}
	}

	return false;
}

int DESC_MANAGER::GetHostConnectionCount(const struct sockaddr_in & c_rSockAddr)
{
	int iCount = 0;
	for (const auto & rRec : m_set_pkDesc)
	{
		if (rRec->GetHostName() == inet_ntoa(c_rSockAddr.sin_addr))
			iCount++;
	}

	return iCount;
}

void DESC_MANAGER::RegisterInstrusiveConnection(const std::string & sHost)
{
	static const int MAXIMUM_HANDSHAKE_LIMIT = 3;
	static const DWORD HANDSHAKE_DELAY = 1500;

	auto fIt = m_connection_mapper.find(sHost);
	if (fIt == m_connection_mapper.end())
		fIt = m_connection_mapper.emplace(std::piecewise_construct, std::forward_as_tuple(sHost), std::forward_as_tuple(0, 0, false)).first;

	std::get<1>(fIt->second)++;
	if (std::get<0>(fIt->second) >= get_dword_time())
	{
		if (std::get<1>(fIt->second) >= MAXIMUM_HANDSHAKE_LIMIT)
			std::get<2>(fIt->second) = true;
	}
	else
	{
		std::get<0>(fIt->second) = get_dword_time()+HANDSHAKE_DELAY;
		std::get<1>(fIt->second) = 0;
	}
}

bool DESC_MANAGER::IsIntrusiveConnection(const std::string & sHost)
{
	auto fIt = m_connection_mapper.find(sHost);
	if (fIt == m_connection_mapper.end())
		return false;

	return std::get<2>(fIt->second);
}

void DESC_MANAGER::AddToHandshakeWhiteList(const TPacketGGHandshakeValidate * pack)
{
	if (g_bAuthServer)
		return;

	s_handshake_whitelist.insert(pack->sUserIP);
}

bool DESC_MANAGER::IsOnHandshakeWhitelist(const struct sockaddr_in & c_rSockAddr)
{
	return (s_handshake_whitelist.find(inet_ntoa(c_rSockAddr.sin_addr)) != s_handshake_whitelist.end());
}

void DESC_MANAGER::ConnectionCollector()
{
	static const DWORD HANDSHAKE_ELAPSE_TIME = 5;

	std::unordered_set<LPDESC> s_garbage;
	for (const auto & rRec : m_map_handshake)
	{
		if (rRec.second->IsPhase(PHASE_HANDSHAKE) && rRec.second->GetCreationTime()+HANDSHAKE_ELAPSE_TIME < get_global_time())
			s_garbage.insert(rRec.second);
	}

	std::for_each(s_garbage.begin(), s_garbage.end(), [this](const LPDESC & rDesc) { DestroyDesc(rDesc, true); });
}
#endif