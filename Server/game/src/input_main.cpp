#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
#include "67bonusnew.h"
#endif
#ifdef OFFLINE_SHOP
#include "offlineshop.h"
#include "offlineshop_manager.h"
#endif
#include "cmd.h"
#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
#endif
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "banword.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "HackShield.h"
#include "XTrapManager.h"
#include "DragonSoul.h"
#ifdef NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"
#endif
#include "belt_inventory_helper.h" // @fixme119
#include "../../common/CommonDefines.h"
#include "../common/VnumHelper.h"
#include "input.h"
#ifdef ENABLE_DECORUM
#include "decorum_arena.h"
#endif
#ifdef __INGAME_WIKI__
	#include "mob_manager.h"
#endif

#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_CHAT_LOGGING
#define ENABLE_CHAT_SPAMLIMIT
#define ENABLE_WHISPER_CHAT_SPAMLIMIT
#define ENABLE_CHECK_GHOSTMODE

#ifdef ENABLE_CHAT_LOGGING
static char	__escape_string[1024];
static char	__escape_string2[1024];
#endif


#ifdef ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
enum price_switch_bot
{
	BONUS_CHANGER_PRICE_IN_SWITCH_BOT = 2000000, //cambiare qui per il prezzo degli incanta nello switchbot
	BONUS_CHANGER_ITEM_VALUE = 11074,			//cambiare qui per cambiare value degli incanta oggetto
};

#endif


static int __deposit_limit()
{
	return (1000*10000); // 1천만
}

#ifdef TARGET_INFORMATION_SYSTEM
void CInputMain::TargetInfoLoad(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGTargetInfoLoad* p = (TPacketCGTargetInfoLoad*)c_pData;
	TPacketGCTargetInfo pInfo;
	pInfo.header = HEADER_GC_TARGET_INFO;
	static std::vector<LPITEM> s_vec_item;
	s_vec_item.clear();
	LPITEM pkInfoItem;
	LPCHARACTER m_pkChrTarget = CHARACTER_MANAGER::instance().Find(p->dwVID);
	if (!ch || !m_pkChrTarget)
	return;
	// if (m_pkChrTarget && (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()))
	// {
	// if (thecore_heart->pulse - (int) ch->GetLastTargetInfoPulse() < passes_per_sec * 3)
	// return;

	// ch->SetLastTargetInfoPulse(thecore_heart->pulse);

	if (ITEM_MANAGER::instance().CreateDropItemVector(m_pkChrTarget, ch, s_vec_item) && (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()))
	{
		if (s_vec_item.size() == 0);
		else if (s_vec_item.size() == 1)
		{
			pkInfoItem = s_vec_item[0];
			pInfo.dwVID = m_pkChrTarget->GetVID();
			pInfo.race = m_pkChrTarget->GetRaceNum();
			pInfo.dwVnum = pkInfoItem->GetVnum();
			pInfo.count = pkInfoItem->GetCount();
#ifdef ENABLE_SEND_TARGET_INFO_EXTENDED
			pInfo.rarity = pkInfoItem->GetRarity();
#endif
			ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
		}
		else
		{
			int iItemIdx = s_vec_item.size() - 1;
			while (iItemIdx >= 0)
			{
				pkInfoItem = s_vec_item[iItemIdx--];
				if (!pkInfoItem)
				{
					sys_err("pkInfoItem null in vector idx %d", iItemIdx + 1);
					continue;
				}
				pInfo.dwVID = m_pkChrTarget->GetVID();
				pInfo.race = m_pkChrTarget->GetRaceNum();
				pInfo.dwVnum = pkInfoItem->GetVnum();
				pInfo.count = pkInfoItem->GetCount();
#ifdef ENABLE_SEND_TARGET_INFO_EXTENDED
				pInfo.rarity = pkInfoItem->GetRarity();
#endif
				ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
			}
		}
	}
}
#endif
void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"채팅 금지 상태입니다."));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

	char buf[128+1];

	if (hour > 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d 시간 %d 분 %d 초 동안 채팅금지 상태입니다"), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d 시간 %d 초 동안 채팅금지 상태입니다"), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d 분 %d 초 동안 채팅금지 상태입니다"), min, sec);
	else
		snprintf(buf, sizeof(buf), LC_TEXT("%d 초 동안 채팅금지 상태입니다"), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH+1];

	spam_event_info()
	{
		::memset( host, 0, MAX_HOST_LENGTH+1 );
	}
};

typedef boost::unordered_map<std::string, std::pair<unsigned int, LPEVENT> > spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "block_chat_by_ip_event> <Factor> Null pointer" );
		return 0;
	}

	const char * host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.insert(std::make_pair(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT) NULL)));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char * word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);

			LogManager::instance().CharLog(ch, 0, "SPAM", word);

			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char * src, int maxLen, int & tagLen, std::string & extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char * cur = ++src;

	if (*cur == '|') // ||는 |로 표시한다.
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H') // hyperlink |Hitem:10000:0:0:0:0|h[이름]|h
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char * src, int src_len, int & hyperlinks, bool & colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char * c_pszText, size_t len)
{
	//개인상점중에 금강경을 사용할 경우
	//0 : 정상적으로 사용
	//1 : 금강경 부족
	//2 : 금강경이 있으나, 개인상점에서 사용중
	//3 : 기타
	//4 : 에러
	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

#ifdef ENABLE_NEWSTUFF
	if (g_bDisablePrismNeed)
		return 0;
#endif
	int nPrismCount = ch->CountSpecifyItem(ITEM_PRISM);

	if (nPrismCount < hyperlinks)
		return 1;


	if (!ch->GetMyShop())
	{
		ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
		return 0;
	} else
	{
		int sellingNumber = ch->GetMyShop()->GetNumberByVnum(ITEM_PRISM);
		if(nPrismCount - sellingNumber < hyperlinks)
		{
			return 2;
		} else
		{
			ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
			return 0;
		}
	}

	return 4;
}

int CInputMain::Whisper(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

#ifdef ENABLE_WHISPER_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 12)
	{
		if (ch->GetChatCounter() == 18)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}	
#endif

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"채팅 금지 상태입니다."));
		return (iExtraLen);
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch)
		return (iExtraLen);

	LPDESC pkDesc = NULL;

	BYTE bOpponentEmpire = 0;

	if (test_server)
	{
		if (!pkChr)
			sys_log(0, "Whisper to %s(%s) from %s", "Null", pinfo->szNameTo, ch->GetName());
		else
			sys_log(0, "Whisper to %s(%s) from %s", pkChr->GetName(), pinfo->szNameTo, ch->GetName());
	}

	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
			bOpponentEmpire = pkCCI->bEmpire;

			if (test_server)
				sys_log(0, "Whisper to %s from %s (Channel %d Mapindex %d)", "Null", ch->GetName(), pkCCI->bChannel, pkCCI->lMapIndex);
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
		bOpponentEmpire = pkChr->GetEmpire();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;

			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_NOT_EXIST;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
#ifdef ENABLE_WHISPER_RENEWAL			
			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			if (!(std::string(buf).find("|?whisper_renewal>|") != std::string::npos || std::string(buf).find("|?whisper_renewal<|") != std::string::npos)) {
				ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
				sys_log(0, "WHISPER: no player");
			}
#else
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
			sys_log(0, "WHISPER: no player");
#endif
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_TARGET_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		#ifdef ENABLE_MESSENGER_BLOCK
		else if (pkChr && MessengerManager::instance().CheckMessengerList(ch->GetName(), pkChr->GetName(), SYST_BLOCK))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;

				char msg_2[CHAT_MAX_LEN + 1];
				snprintf(msg_2, sizeof(msg_2), LC_TEXT("%s'yi blokladim"), pkChr->GetName());
				int len = MIN(CHAT_MAX_LEN, strlen(msg_2) + 1);

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				pack.bType = WHISPER_TYPE_SYSTEM;
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

				TEMP_BUFFER buf;

				buf.write(&pack, sizeof(TPacketGCWhisper));
				buf.write(msg_2, len);
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());
			}
		}
		#endif
		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;

			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			if (true == SpamBlockCheck(ch, buf, buflen))
			{
				if (!pkChr)
				{
					CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

					if (pkCCI)
					{
						pkDesc->SetRelay("");
					}
				}
				return iExtraLen;
			}

			CBanwordManager::instance().ConvertString(buf, buflen);

			if (g_bEmpireWhisper)
				if (!ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
					if (!(pkChr && pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)))
						if (bOpponentEmpire != ch->GetEmpire() && ch->GetEmpire() && bOpponentEmpire // 서로 제국이 다르면서
								&& ch->GetGMLevel() == GM_PLAYER && gm_get_level(pinfo->szNameTo) == GM_PLAYER) // 둘다 일반 플레이어이면
							// 이름 밖에 모르니 gm_get_level 함수를 사용
						{
							if (!pkChr)
							{
								// 다른 서버에 있으니 제국 표시만 한다. bType의 상위 4비트를 Empire번호로 사용한다.
								bType = ch->GetEmpire() << 4;
							}
							else
							{
								ConvertEmpireText(ch->GetEmpire(), buf, buflen, 10 + 2 * pkChr->GetSkillPower(SKILL_LANGUAGE1 + ch->GetEmpire() - 1)/*변환확률*/);
							}
						}

			int processReturn = ProcessTextTag(ch, buf, buflen);
			if (0!=processReturn)
			{
				if (ch->GetDesc())
				{
					TItemTable * pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

					if (pTable)
					{
						char buf[128];
						int len;
						#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
						if (3==processReturn) //교환중
							len = snprintf(buf, sizeof(buf), LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."), pTable->szLocaleName[ch->GetLanguage()]);
						else
							len = snprintf(buf, sizeof(buf), LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s이 필요합니다."), pTable->szLocaleName[ch->GetLanguage()]);
						#else
						if (3==processReturn) //교환중
							len = snprintf(buf, sizeof(buf), LC_TEXT("사용할수 없습니다."), pTable->szLocaleName);
						else
							len = snprintf(buf, sizeof(buf), LC_TEXT("%s이 필요합니다."), pTable->szLocaleName);
						#endif

						if (len < 0 || len >= (int) sizeof(buf))
							len = sizeof(buf) - 1;

						++len;  // \0 문자 포함

						TPacketGCWhisper pack;

						pack.bHeader = HEADER_GC_WHISPER;
						pack.bType = WHISPER_TYPE_ERROR;
						pack.wSize = sizeof(TPacketGCWhisper) + len;
						strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

						ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
						ch->GetDesc()->Packet(buf, len);

						#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
						sys_log(0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName[ch->GetLanguage()], ch->GetName());
						#else
						sys_log(0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName, ch->GetName());
						#endif
					}
				}

				// 릴래이 상태일 수 있으므로 릴래이를 풀어준다.
				pkDesc->SetRelay("");
				return (iExtraLen);
			}

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				
#ifdef ENABLE_AFK_MODE_SYSTEM
				if (pkChr && pkChr->IsAway())
				{
					if (ch->GetDesc())
					{
						TPacketGCWhisper pack;
						char msg_2[CHAT_MAX_LEN + 1];

						int timeReal = time(0)-pkChr->GetLastPacketTime();
						int second = timeReal % 60;
						int minute = (timeReal / 60) % 60;
						int hour = ((timeReal / 60) / 60) % 24;
	
						snprintf(msg_2, sizeof(msg_2), "%s e' AFK da: %02d:%02d:%02d",pkChr->GetName(),hour,minute,second);
						int len = MIN(CHAT_MAX_LEN, strlen(msg_2) + 1);
						pack.bHeader = HEADER_GC_WHISPER;
						pack.wSize = sizeof(TPacketGCWhisper) + len;
						pack.bType = WHISPER_TYPE_SYSTEM;
						strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

						TEMP_BUFFER buf;
						buf.write(&pack, sizeof(TPacketGCWhisper));
						buf.write(msg_2, len);
						ch->GetDesc()->Packet(buf.read_peek(), buf.size());
					}
				}
#endif

				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
				std::string c_pszEmpire = ConvertEmpireIndexToStr(ch->GetEmpire()).c_str();		
				std::string c_pszLanguage = GetLanguageFlag(ch->GetName()).c_str();
				
				strlcpy(pack.bLanguage, c_pszLanguage.c_str(), sizeof(pack.bLanguage));
				strlcpy(pack.bEmpire, c_pszEmpire.c_str(), sizeof(pack.bEmpire));
#endif
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				// desc->BufferedPacket을 하지 않고 버퍼에 써야하는 이유는
				// P2P relay되어 패킷이 캡슐화 될 수 있기 때문이다.
				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());

				// @warme006
				// sys_log(0, "WHISPER: %s -> %s : %s", ch->GetName(), pinfo->szNameTo, buf);
#ifdef ENABLE_CHAT_LOGGING
				if (ch->IsGM())
				{
					LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), buf, buflen);
					LogManager::instance().EscapeString(__escape_string2, sizeof(__escape_string2), pinfo->szNameTo, sizeof(pack.szNameFrom));
					LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, __escape_string2, "WHISPER", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
				}
#endif
			}
		}
	}
	if(pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void * m_buf;
	int	m_buf_len;

	RawPacketToCharacterFunc(const void * buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;
	char converted_msg[CHAT_MAX_LEN+1];

	BYTE bEmpire;
	int iMapIndex;
	int namelen;

	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, BYTE bEmpire, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), bEmpire(bEmpire), iMapIndex(iMapIndex), namelen(iNameLen)
	{
		memset( converted_msg, 0, sizeof(converted_msg) );
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

		d->BufferedPacket(&p, sizeof(packet_chat));

		if (d->GetEmpire() == bEmpire ||
			bEmpire == 0 ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			d->Packet(orig_msg, orig_len);
		}
		else
		{
			// 사람마다 스킬레벨이 다르니 매번 해야합니다
			size_t len = strlcpy(converted_msg, orig_msg, sizeof(converted_msg));

			if (len >= sizeof(converted_msg))
				len = sizeof(converted_msg) - 1;

			ConvertEmpireText(bEmpire, converted_msg + namelen, len - namelen, 10 + 2 * d->GetCharacter()->GetSkillPower(SKILL_LANGUAGE1 + bEmpire - 1));
			d->Packet(converted_msg, orig_len);
		}
	}
};

struct FYmirChatPacket
{
	packet_chat& packet;
	const char* m_szChat;
	size_t m_lenChat;
	const char* m_szName;

	int m_iMapIndex;
	BYTE m_bEmpire;
	bool m_ring;

	char m_orig_msg[CHAT_MAX_LEN+1];
	int m_len_orig_msg;
	char m_conv_msg[CHAT_MAX_LEN+1];
	int m_len_conv_msg;

	FYmirChatPacket(packet_chat& p, const char* chat, size_t len_chat, const char* name, size_t len_name, int iMapIndex, BYTE empire, bool ring)
		: packet(p),
		m_szChat(chat), m_lenChat(len_chat),
		m_szName(name),
		m_iMapIndex(iMapIndex), m_bEmpire(empire),
		m_ring(ring)
	{
		m_len_orig_msg = snprintf(m_orig_msg, sizeof(m_orig_msg), "%s : %s", m_szName, m_szChat) + 1; // 널 문자 포함

		if (m_len_orig_msg < 0 || m_len_orig_msg >= (int) sizeof(m_orig_msg))
			m_len_orig_msg = sizeof(m_orig_msg) - 1;

		m_len_conv_msg = snprintf(m_conv_msg, sizeof(m_conv_msg), "??? : %s", m_szChat) + 1; // 널 문자 미포함

		if (m_len_conv_msg < 0 || m_len_conv_msg >= (int) sizeof(m_conv_msg))
			m_len_conv_msg = sizeof(m_conv_msg) - 1;

		ConvertEmpireText(m_bEmpire, m_conv_msg + 6, m_len_conv_msg - 6, 10); // 6은 "??? : "의 길이
	}

	void operator() (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != m_iMapIndex)
			return;

		if (m_ring ||
			d->GetEmpire() == m_bEmpire ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			packet.size = m_len_orig_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_orig_msg, m_len_orig_msg);
		}
		else
		{
			packet.size = m_len_conv_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_conv_msg, m_len_conv_msg);
		}
	}
};

#ifdef NEW_PET_SYSTEM
void CInputMain::BraveRequestPetName(LPCHARACTER ch, const char* c_pData)
{
	if (!ch->GetDesc()) { return; }
	int vid = ch->GetEggVid();
	if (vid == 0) { return; }

	TPacketCGRequestPetName* p = (TPacketCGRequestPetName*)c_pData;

	if (ch->GetGold() < 100000) {
		ch->ChatPacket(CHAT_TYPE_INFO, "Hai bisogno di 100.000 yang");
	}

	if (ch->CountSpecifyItem(vid) > 0 && check_name(p->petname) != 0) {
		DBManager::instance().SendMoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), -100000);
		ch->PointChange(POINT_GOLD, -100000, true);
		ch->RemoveSpecifyItem(vid, 1);
		LPITEM item = ch->AutoGiveItem(vid + 300, 1);
		int tmpslot = number(1, 3);
		int tmpskill[3] = { 0, 0, 0 };
		for (int i = 0; i < 3; ++i)
		{
			if (i > tmpslot - 1)
				tmpskill[i] = -1;
		}
		int tmpdur = number(1, 14) * 24 * 60;
		char szQuery1[1024];
		snprintf(szQuery1, sizeof(szQuery1), "INSERT INTO new_petsystem VALUES(%lu,'%s', 1, 0, 0, 0, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, 0)", item->GetID(), p->petname, number(1, 23), number(1, 23), number(1, 23), tmpskill[0], 0, tmpskill[1], 0, tmpskill[2], 0, tmpdur, tmpdur);
		std::auto_ptr<SQLMsg> pmsg2(DBManager::instance().DirectQuery(szQuery1));
		ch->ChatPacket(CHAT_TYPE_INFO, "Hai dato il nome al tuo pet.");
	}
	else {
		ch->ChatPacket(CHAT_TYPE_INFO, "Error setting name.");
	}
}
#endif

int CInputMain::Chat(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

	if (uiBytes < pinfo->size)
		return -1;

	const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN + 3) + 1];
	strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return iExtraLen;
	}
#ifdef ENABLE_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 6)
	{
		if (ch->GetChatCounter() == 12)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}
#else
	if (ch->IncreaseChatCounter() >= 10)
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	// 채팅 금지 Affect 처리
	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return iExtraLen;
	}

	if (true == SpamBlockCheck(ch, buf, buflen))
	{
		return iExtraLen;
	}

	// @fixme133 begin
	CBanwordManager::instance().ConvertString(buf, buflen);

	int processReturn = ProcessTextTag(ch, buf, buflen);
	if (0!=processReturn)
	{
		const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

		if (NULL != pTable)
		{
			#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
			if (3==processReturn) //교환중
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."), pTable->szLocaleName[ch->GetLanguage()]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s이 필요합니다."), pTable->szLocaleName[ch->GetLanguage()]);
			#else
			if (3==processReturn) //교환중
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"사용할수 없습니다."), pTable->szLocaleName);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s이 필요합니다."), pTable->szLocaleName);
			#endif
		}

		return iExtraLen;
	}
	// @fixme133 end

	char chatbuf[CHAT_MAX_LEN + 1];

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	int len;
	const char* kingdoms[3] = {"","",""};
	len = snprintf(chatbuf, sizeof(chatbuf), "%s %s : %s", kingdoms[ch->GetEmpire() - 1], ch->GetName(), buf);
#else
	strncat(chatbuf, ch->GetName(), CHAT_MAX_LEN - strlen(chatbuf));
	strncat(chatbuf, " : ", CHAT_MAX_LEN - strlen(chatbuf));
	strncat(chatbuf, buf, CHAT_MAX_LEN - strlen(chatbuf));
	
	int len = strlen(chatbuf);
#endif
	
	// ### CHAT_FORMAT END

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	if (CHAT_TYPE_SHOUT == pinfo->type)
	{
		std::string c_pszEmpire = ConvertEmpireIndexToStr(ch->GetEmpire()).c_str();		
		std::string c_pszLanguage = GetLanguageFlag(ch->GetName()).c_str();			
		std::string temp_buf = chatbuf;

		if (strlen(c_pszEmpire.c_str()) > 0)
		{
			len = snprintf(chatbuf, sizeof(chatbuf), "/lang/%s//empire/%s/%s", c_pszLanguage.c_str(), c_pszEmpire.c_str(), temp_buf.c_str());
		}
	}
#else
	if (CHAT_TYPE_SHOUT == pinfo->type)
	{
		LogManager::instance().ShoutLog(g_bChannel, ch->GetEmpire(), chatbuf);
	}
#endif

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type == CHAT_TYPE_SHOUT)
	{
		// const int SHOUT_LIMIT_LEVEL = 15;

		if (ch->GetLevel() < g_iShoutLimitLevel)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"외치기는 레벨 %d 이상만 사용 가능 합니다."), g_iShoutLimitLevel);
			return (iExtraLen);
		}

		// if (thecore_heart->pulse - (int) ch->GetLastShoutPulse() < passes_per_sec * g_iShoutLimitTime)
		if (thecore_heart->pulse - (int) ch->GetLastShoutPulse() < passes_per_sec * 15)
			return (iExtraLen);

		ch->SetLastShoutPulse(thecore_heart->pulse);

		TPacketGGShout p;

		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = ch->GetEmpire();
		strlcpy(p.szText, chatbuf, sizeof(p.szText));

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));

		SendShout(chatbuf, ch->GetEmpire());

		return (iExtraLen);
	}

	TPacketGCChat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = pinfo->type;
	pack_chat.id = ch->GetVID();

	switch (pinfo->type)
	{
		case CHAT_TYPE_TALKING:
			{
				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

				if (false)
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FYmirChatPacket(pack_chat,
								buf,
								strlen(buf),
								ch->GetName(),
								strlen(ch->GetName()),
								ch->GetMapIndex(),
								ch->GetEmpire(),
								ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)));
				}
				else
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FEmpireChatPacket(pack_chat,
								chatbuf,
								len,
								(ch->GetGMLevel() > GM_PLAYER ||
								 ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)) ? 0 : ch->GetEmpire(),
								ch->GetMapIndex(), strlen(ch->GetName())));
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, "", "NORMAL", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		case CHAT_TYPE_PARTY:
			{
				if (!ch->GetParty())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"파티 중이 아닙니다."));
				else
				{
					TEMP_BUFFER tbuf;

					tbuf.write(&pack_chat, sizeof(pack_chat));
					tbuf.write(chatbuf, len);

					RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
					ch->GetParty()->ForEachOnlineMember(f);
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetParty()->GetLeaderPID(), "", "PARTY", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		case CHAT_TYPE_GUILD:
			{
				if (!ch->GetGuild())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"길드에 가입하지 않았습니다."));
				else
				{
					ch->GetGuild()->Chat(chatbuf);
#ifdef ENABLE_CHAT_LOGGING
					if (ch->IsGM())
					{
						LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
						LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetGuild()->GetID(), ch->GetGuild()->GetName(), "GUILD", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
					}
#endif
				}
			}
			break;

		default:
			sys_err("Unknown chat type %d", pinfo->type);
			break;
	}

	return (iExtraLen);
}

void CInputMain::ItemUse(LPCHARACTER ch, const char * data)
{
	ch->UseItem(((struct command_item_use *) data)->Cell);
}

void CInputMain::ItemToItem(LPCHARACTER ch, const char * pcData)
{
	TPacketCGItemUseToItem * p = (TPacketCGItemUseToItem *) pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

void CInputMain::ItemDrop(LPCHARACTER ch, const char * data)
{
	struct command_item_drop * pinfo = (struct command_item_drop *) data;

	//MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	//END_MONARCH_LIMIT
	if (!ch)
		return;

	// 엘크가 0보다 크면 엘크를 버리는 것 이다.
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char * data)
{
	//MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	//END_MONARCH_LIMIT

	TPacketCGItemDrop2 * pinfo = (TPacketCGItemDrop2 *) data;

	// 엘크가 0보다 크면 엘크를 버리는 것 이다.

	if (!ch)
		return;
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell, pinfo->count);
}

#ifdef ENABLE_SELL_ITEM
void CInputMain::ItemSell(LPCHARACTER ch, const char * data)
{
	TPacketCGItemSell * pinfo = (TPacketCGItemSell *) data;
	
	if (!ch)
		return;

	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->SellItem(pinfo->Cell);
}
#endif


void CInputMain::ItemMove(LPCHARACTER ch, const char * data)
{
	if (!ch)
		return;

	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

#if defined(__BL_SOUL_ROULETTE__)
void CInputMain::SoulRoulette(LPCHARACTER ch, const char* data)
{
	if (!ch)
		return;

	const TPacketCGSoulRoulette* pinfo = reinterpret_cast<const TPacketCGSoulRoulette*>(data);
	enum { CLOSE, TURN, GIVE };

	switch (pinfo->option) {
	case CLOSE:
		if (ch->GetSoulRoulette()) {
			if (ch->GetSoulRoulette()->GetGiftVnum())
				ch->ChatPacket(CHAT_TYPE_INFO, "Please wait, <Soul Roulette> is active now.");
			else
			{
				ch->GetSoulRoulette()->SendPacket(CSoulRoulette::Packet::CLOSE);
				ch->SetSoulRoulette(NULL);
			}
		}
		break;
	case TURN:
		if (ch->GetSoulRoulette())
			ch->GetSoulRoulette()->TurnWheel();
		break;
	case GIVE:
		if (ch->GetSoulRoulette())
			ch->GetSoulRoulette()->GiveGift();
		break;
	default:
		sys_err("CInputMain::SoulRoulette : Unknown option %d : %s", pinfo->option, ch->GetName());
		break;
	}
}
#endif


#ifdef ENABLE_SORT_INVEN
void CInputMain::SortInven(LPCHARACTER ch, const char * data)
{
	TPacketCGSortInven * pinfo = (TPacketCGSortInven *) data;
	if (ch) ch->SortInven(pinfo->option);
}
#endif

#ifdef NEW_ADD_INVENTORY
void CInputMain::Envanter_pakettiii(LPCHARACTER ch, const char * data)
{
    if (ch)
        ch->Envanter_update();
}
#endif

void CInputMain::ItemPickup(LPCHARACTER ch, const char * data)
{
	struct command_item_pickup * pinfo = (struct command_item_pickup*) data;
	if (ch)
#ifdef ENABLE_CHECK_PICKUP_HACK
		if (ch->PickupItem(pinfo->vid))
		{
			DWORD d_difference = get_dword_time() - ch->GetLastPickupTime();
			if (d_difference < ENABLE_CHECK_PICKUP_HACK && d_difference > 3)
			{
				LPDESC d = ch->GetDesc();
				if (d)
				{
					if (d->DelayedDisconnect(3))
					{
						LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: Pickup Hack", ch);
					}
				}
			}
			ch->SetLastPickupTime();
		}
#else
		ch->PickupItem(pinfo->vid);
#endif
}


void CInputMain::QuickslotAdd(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_add * pinfo = (struct command_quickslot_add *) data;
#ifdef ENABLE_FIX_QUICK_SLOT	
	if(pinfo->slot.type == QUICKSLOT_TYPE_ITEM)
	{
		LPITEM item = NULL;
		
		TItemPos srcCell(INVENTORY, pinfo->slot.pos);
		
		if (!(item = ch->GetItem(srcCell)))
			return;
		
		if (item->GetType() != ITEM_USE && item->GetType() != ITEM_QUEST)
			return;
	}
#endif
	
	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_del * pinfo = (struct command_quickslot_del *) data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_swap * pinfo = (struct command_quickslot_swap *) data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*) c_pData;
	
	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
		#ifdef ENABLE_MESSENGER_BLOCK
		case MESSENGER_SUBHEADER_CG_ADD_BLOCK_BY_VID:
			{
				if (uiBytes < sizeof(TPacketCGMessengerAddBlockByVID))
					return -1;

				TPacketCGMessengerAddBlockByVID * p2 = (TPacketCGMessengerAddBlockByVID *) c_pData;
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

				if (!ch_companion)
					return sizeof(TPacketCGMessengerAddBlockByVID);

				if (ch->IsObserverMode())
					return sizeof(TPacketCGMessengerAddBlockByVID);

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch_companion->GetGuild() == ch->GetGuild() && ch->GetGuild() != NULL && ch_companion->GetGuild() != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"lonca olmaz"));
					return sizeof(TPacketCGMessengerAddBlockByVID);
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"arkadas ekleyemezsin."));
					return sizeof(TPacketCGMessengerAddBlockByVID);
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"zaten bloklu"));
					return sizeof(TPacketCGMessengerAddBlockByVID);
				}
				
				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"gm'ye bu yanlis yapilmaz."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				if (ch->GetDesc() == d) // 자신은 추가할 수 없다.
					return sizeof(TPacketCGMessengerAddBlockByVID);

				MessengerManager::instance().AddToBlockList(ch->GetName(), ch_companion->GetName());
			}
			return sizeof(TPacketCGMessengerAddBlockByVID);

		case MESSENGER_SUBHEADER_CG_ADD_BLOCK_BY_NAME:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(name, c_pData, sizeof(name));
				
				if (gm_get_level(name) != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"gm'ye bu yanlis yapilmaz."));
					return CHARACTER_NAME_MAX_LEN;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);
				
				if (!tch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s yok ki."), name);
				else
				{
					if (tch == ch) // 자신은 추가할 수 없다.
						return CHARACTER_NAME_MAX_LEN;
						
					if (tch->GetGuild() == ch->GetGuild() && ch->GetGuild() != NULL && tch->GetGuild() != NULL)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"lonca olmaz"));
						return CHARACTER_NAME_MAX_LEN;
					}
					
					if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_FRIEND))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"arkadas ekleyemezsin."));
						return CHARACTER_NAME_MAX_LEN;
					}
						
					MessengerManager::instance().AddToBlockList(ch->GetName(), tch->GetName());
				}
			}
			return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_REMOVE_BLOCK:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char char_name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(char_name, c_pData, sizeof(char_name));
				
				if (!MessengerManager::instance().CheckMessengerList(ch->GetName(), char_name, SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"bloklu degil"));
					return CHARACTER_NAME_MAX_LEN;
				}
				MessengerManager::instance().RemoveFromBlockList(ch->GetName(), char_name);
			}
			return CHARACTER_NAME_MAX_LEN;
		#endif
		
		case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
			{
				if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
					return -1;

				TPacketCGMessengerAddByVID * p2 = (TPacketCGMessengerAddByVID *) c_pData;
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

				if (!ch_companion)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->IsObserverMode())
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"상대방이 메신져 추가 거부 상태입니다."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<메신져> 운영자는 메신져에 추가할 수 없습니다."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				if (ch->GetDesc() == d) // 자신은 추가할 수 없다.
					return sizeof(TPacketCGMessengerAddByVID);
					
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"zaten arkadas."));
					return sizeof(TPacketCGMessengerAddByVID);
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"blok ekleyemezsin"));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				MessengerManager::instance().RequestToAdd(ch, ch_companion);
			}
			return sizeof(TPacketCGMessengerAddByVID);

		case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(name, c_pData, sizeof(name));

				if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<메신져> 운영자는 메신져에 추가할 수 없습니다."));
					return CHARACTER_NAME_MAX_LEN;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

				if (!tch)
				{
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
				MessengerManager::instance().P2PRequestToAdd_Stage1(ch, name);
#endif
				//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not online."), name);
				}
				else
				{
					if (tch == ch) // 자신은 추가할 수 없다.
						return CHARACTER_NAME_MAX_LEN;

					if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"상대방이 메신져 추가 거부 상태입니다."));
					}
					else
					{
						if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_FRIEND))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"zaten arkadas."));
							return CHARACTER_NAME_MAX_LEN;
						}
						
						if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_BLOCK))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"blok ekleyemezsin"));
							return CHARACTER_NAME_MAX_LEN;
						}
						// 메신저가 캐릭터단위가 되면서 변경
						MessengerManager::instance().RequestToAdd(ch, tch);
					}
				}
			}
			return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_REMOVE:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				char char_name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(char_name, c_pData, sizeof(char_name));
				if (!MessengerManager::instance().CheckMessengerList(ch->GetName(), char_name, SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"arkadas degil"));
					return CHARACTER_NAME_MAX_LEN;
				}
				MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
				MessengerManager::Instance().RemoveFromList(char_name, ch->GetName());	//@fixme485
			}
			return CHARACTER_NAME_MAX_LEN;

		default:
			sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

int CInputMain::Shop(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	TPacketCGShop * p = (TPacketCGShop *) data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (test_server)
		sys_log(0, "CInputMain::Shop() ==> SubHeader %d", p->subheader);

	const char * c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
		case SHOP_SUBHEADER_CG_END:
			sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
			CShopManager::instance().StopShopping(ch);
			return 0;

		case SHOP_SUBHEADER_CG_BUY:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
					return -1;

				BYTE bPos = *(c_pData + 1);
				sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);
				CShopManager::instance().Buy(ch, bPos);
				return (sizeof(BYTE) + sizeof(BYTE));
			}

		case SHOP_SUBHEADER_CG_SELL:
			{
				if (uiBytes < sizeof(BYTE))
					return -1;

				BYTE pos = *c_pData;

				sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
				CShopManager::instance().Sell(ch, pos);
				return sizeof(BYTE);
			}
#ifdef __SPECIAL_STORAGE_SYSTEM__
		case SHOP_SUBHEADER_CG_SELL2:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(BYTE) + sizeof(BYTE))
					return -1;

				const BYTE wPos = *reinterpret_cast<const BYTE*>(c_pData);
				const BYTE bCount = *(c_pData + sizeof(BYTE));
				const BYTE bType = *(c_pData + sizeof(BYTE) + sizeof(BYTE));

				sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
				CShopManager::instance().Sell(ch, wPos, bCount, bType);
				return sizeof(BYTE) + sizeof(BYTE) + sizeof(BYTE);
			}
#else
		case SHOP_SUBHEADER_CG_SELL2:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
					return -1;

				BYTE pos = *(c_pData++);
				BYTE count = *(c_pData);

				sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
				CShopManager::instance().Sell(ch, pos, count);
				return sizeof(BYTE) + sizeof(BYTE);
			}
#endif

		default:
			sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

#ifdef OFFLINE_SHOP
int CInputMain::OfflineShop(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	TPacketCGShop * p = (TPacketCGShop *)data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (test_server)
		sys_log(0, "CInputMain::OfflineShop ==> SubHeader %d", p->subheader);

	const char * c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);
#ifdef WJ_SECURITY_SYSTEM
	if (ch->IsActivateSecurity() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot offlineshop with security key activate"));
		return 0;
	}
#endif
	switch (p->subheader)
	{
	case SHOP_SUBHEADER_CG_END:
	{
		sys_log(1, "INPUT: %s OFFLINE_SHOP: END", ch->GetName());
		COfflineShopManager::instance().StopShopping(ch);
		return 0;
	}
	case SHOP_SUBHEADER_CG_BUY:
	{
		if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
			return -1;

		if (ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Fii sigur ca nu ai alte ferestre deschise!"));
			return 0;
		}

		BYTE bPos = *(c_pData + 1);
		sys_log(1, "INPUT: %s OFFLINE_SHOP: BUY %d", ch->GetName(), bPos);
		COfflineShopManager::instance().Buy(ch, bPos);
		return (sizeof(BYTE) + sizeof(BYTE));
	}
	case SHOP_SUBHEADER_CG_PUT_ITEM:
	{
		if (uiBytes < sizeof(TOfflineShopItemTable))
			return -1;

		TOfflineShopItemTable * pTable = (TOfflineShopItemTable *)(c_pData);
		// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
		COfflineShopManager::instance().PutItem(ch, pTable->item_pos, pTable->price, pTable->cheque);
#else
		COfflineShopManager::instance().PutItem(ch, pTable->item_pos, pTable->price);
#endif
		// END_OF_MAX_YANG
		return (sizeof(TOfflineShopItemTable));
	}
	case SHOP_SUBHEADER_CG_EDIT_ITEM:
	{
		if (uiBytes < sizeof(TOfflineShopItemTable))
			return -1;

		TOfflineShopItemTable * pTable = (TOfflineShopItemTable *)(c_pData);
		// BEGIN_MAX_YANG
#ifdef ENABLE_CHEQUE_SYSTEM
		COfflineShopManager::instance().EditPriceItem(ch, pTable->iPos, pTable->price, pTable->cheque);
#else
		COfflineShopManager::instance().EditPriceItem(ch, pTable->iPos, pTable->price);
#endif
		// END_OF_MAX_YANG
		return (sizeof(TOfflineShopItemTable));
	}
	case SHOP_SUBHEADER_CG_DEL_ITEM:
	{
		if (uiBytes < sizeof(TOfflineShopItemTable))
			return -1;

		TOfflineShopItemTable * pTable = (TOfflineShopItemTable *)(c_pData);
		COfflineShopManager::instance().DelSingleItem(ch, pTable->iPos);
		return (sizeof(TOfflineShopItemTable));
	}
	case SHOP_SUBHEADER_CG_DESTROY_OFFLINE_SHOP:
	{
		sys_log(1, "INPUT: %s OFFLINE_SHOP_DESTROY", ch->GetName());
		// COfflineShopManager::instance().DestroyOfflineShop(ch, 0, ch->GetPlayerID(), true);
		COfflineShopManager::instance().AutoCloseOfflineShop(ch);
		return 0;
	}
	case SHOP_SUBHEADER_CG_REFRESH_OFFLINE_SHOP_MONEY:
	{
		sys_log(0, "INPUT: %s SHOP_SUBHEADER_CG_REFRESH_OFFLINE_SHOP_MONEY", ch->GetName());
		COfflineShopManager::instance().RefreshMoney(ch);
		return 0;
	}
	default:
	{
		sys_err("CInputMain::OfflineShop : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}
	}

	return 0;
}
#endif

void CInputMain::OnClick(LPCHARACTER ch, const char * data)
{
	struct command_on_click *	pinfo = (struct command_on_click *) data;
	LPCHARACTER			victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
	else if (test_server)
	{
		sys_err("CInputMain::OnClick %s.Click.NOT_EXIST_VID[%d]", ch->GetName(), pinfo->vid);
	}
}

void CInputMain::Exchange(LPCHARACTER ch, const char * data)
{
	struct command_exchange * pinfo = (struct command_exchange *) data;
	LPCHARACTER	to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(to_ch->GetLanguage(),"거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
			return;
		}

		if( true == to_ch->IsDead() )
		{
			return;
		}
	}

	sys_log(0, "CInputMain()::Exchange()  SubHeader %d ", pinfo->sub_header);

	if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
		return;
	}


	switch (pinfo->sub_header)
	{
		case EXCHANGE_SUBHEADER_CG_START:	// arg1 == vid of target character
			if (!ch->GetExchange())
			{
				if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
				{
					//MONARCH_LIMIT
					/*
					if (to_ch->IsMonarch() || ch->IsMonarch())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"군주와는 거래를 할수가 없습니다"), g_nPortalLimitTime);
						return;
					}
					//END_MONARCH_LIMIT
					*/
					if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);

						if (test_server)
							ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(to_ch->GetLanguage(),"창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);


						if (test_server)
							to_ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, to_ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (ch->GetGold() >= GOLD_MAX)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"액수가 20억 냥을 초과하여 거래를 할수가 없습니다.."));

						sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
						return;
					}

					if (to_ch->IsPC())
					{
						if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
						{
							sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
							return;
						}
					}


#ifdef OFFLINE_SHOP
					if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->GetOfflineShopOwner() || ch->GetMailBox())
#else
					if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen())
#endif
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 거래중일경우 개인상점을 열수가 없습니다."));
						return;
					}

					ch->ExchangeStart(to_ch);
				}
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ITEM_ADD:	// arg1 == position of item, arg2 == position in exchange window
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ITEM_DEL:	// arg1 == position of item
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->RemoveItem(pinfo->arg1);
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
			if (ch->GetExchange())
			{
				const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

				if (GOLD_MAX <= nTotalGold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

					sys_err("[OVERFLOW_GOLD] ELK_ADD (%u) id %u name %s ",
							ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
							ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						   	ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddGold(pinfo->arg1);
			}
			break;

#ifdef ENABLE_CHEQUE_SYSTEM
		case EXCHANGE_SUBHEADER_CG_CHEQUE_ADD:	// arg1 == amount of cheque
			if (ch->GetExchange())
			{
				const int64_t nTotalCheque = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetCheque()) + static_cast<int64_t>(pinfo->arg1);

				if (CHEQUE_MAX <= nTotalCheque)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Celalalt jucator are Won peste limita."));

					sys_err("[OVERFLOW_CHEQUE] CHEQUE_ADD (%u) id %u name %s ",
						ch->GetExchange()->GetCompany()->GetOwner()->GetCheque(),
						ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddCheque(pinfo->arg1);
			}
			break;
#endif

		case EXCHANGE_SUBHEADER_CG_ACCEPT:	// arg1 == not used
			if (ch->GetExchange())
			{
				sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
				ch->GetExchange()->Accept(true);
			}

			break;

		case EXCHANGE_SUBHEADER_CG_CANCEL:	// arg1 == not used
			if (ch->GetExchange())
				ch->GetExchange()->Cancel();
			break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char * data)
{
	struct command_position * pinfo = (struct command_position *) data;

	switch (pinfo->position)
	{
		case POSITION_GENERAL:
			ch->Standup();
			break;

		case POSITION_SITTING_CHAIR:
			ch->Sitdown(0);
			break;

		case POSITION_SITTING_GROUND:
			ch->Sitdown(1);
			break;
	}
}

static const int ComboSequenceBySkillLevel[3][8] =
{
	// 0   1   2   3   4   5   6   7
	{ 14, 15, 16, 17,  0,  0,  0,  0 },
	{ 14, 15, 16, 18, 20,  0,  0,  0 },
	{ 14, 15, 16, 18, 19, 17,  0,  0 },
};

#define COMBO_HACK_ALLOWABLE_MS	100

bool CheckComboHack(LPCHARACTER ch, BYTE bArg, DWORD dwTime, bool CheckSpeedHack)
{
	if(!gHackCheckEnable) return false;

	//	죽거나 기절 상태에서는 공격할 수 없으므로, skip한다.
	//	이렇게 하지 말고, CHRACTER::CanMove()에
	//	if (IsStun() || IsDead()) return false;
	//	를 추가하는게 맞다고 생각하나,
	//	이미 다른 부분에서 CanMove()는 IsStun(), IsDead()과
	//	독립적으로 체크하고 있기 때문에 수정에 의한 영향을
	//	최소화하기 위해 이렇게 땜빵 코드를 써놓는다.
	if (ch->IsStun() || ch->IsDead())
		return false;
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0; // 기본 스칼라 단위 1
#if 0
	sys_log(0, "COMBO: %s arg:%u seq:%u delta:%d checkspeedhack:%d",
			ch->GetName(), bArg, ch->GetComboSequence(), ComboInterval - ch->GetValidComboInterval(), CheckSpeedHack);
#endif
	// bArg 14 ~ 21번 까지 총 8콤보 가능
	// 1. 첫 콤보(14)는 일정 시간 이후에 반복 가능
	// 2. 15 ~ 21번은 반복 불가능
	// 3. 차례대로 증가한다.
	if (bArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// FIXME 첫번째 콤보는 이상하게 빨리 올 수가 있어서 300으로 나눔 -_-;

			// 다수의 몬스터에 의해 다운되는 상황에서 공격을 하면
			// 첫번째 콤보가 매우 적은 인터벌로 들어오는 상황 발생.
			// 이로 인해 콤보핵으로 튕기는 경우가 있어 다음 코드 비 활성화.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 300;

			//sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED),
			//	    ch->IsRiding() ? "yes" : "no");
		}

		ch->SetComboSequence(1);
		ch->SetValidComboInterval((int) (ani_combo_speed(ch, 1) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
		ch->SetLastComboTime(dwTime);
	}
	else if (bArg > 14 && bArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5) // 현재 6콤보 이상은 없다.
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}
		// 자객 쌍수 콤보 예외처리
		else if (bArg == 21 &&
				 idx == 2 &&
				 ch->GetComboSequence() == 5 &&
				 ch->GetJob() == JOB_ASSASSIN &&
				 ch->GetWear(WEAR_WEAPON) &&
				 ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			ch->SetValidComboInterval(300);
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (bArg == 21 && idx == 2 && ch->GetComboSequence() == 5 && ch->GetJob() == JOB_WOLFMAN && ch->GetWear(WEAR_WEAPON) && ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			ch->SetValidComboInterval(300);
#endif
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != bArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
					ch->GetName(),
					bArg,
					ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
					idx,
					ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
						ch->GetName(),
						bArg,
						ComboInterval,
						ch->GetValidComboInterval(),
						ch->GetPoint(POINT_ATT_SPEED),
						ch->IsRiding() ? "yes" : "no");
			}

			// 말을 탔을 때는 15번 ~ 16번을 반복한다
			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

			ch->SetValidComboInterval((int) (ani_combo_speed(ch, bArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
			ch->SetLastComboTime(dwTime);
		}
	}
	else if (bArg == 13) // 기본 공격 (둔갑(Polymorph)했을 때 온다)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// 다수의 몬스터에 의해 다운되는 상황에서 공격을 하면
			// 첫번째 콤보가 매우 적은 인터벌로 들어오는 상황 발생.
			// 이로 인해 콤보핵으로 튕기는 경우가 있어 다음 코드 비 활성화.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

			//sys_log(0, "COMBO_HACK: 6 %s arg:%u interval:%d valid:%u atkspd:%u",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED));
		}

		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{
			// POLYMORPH_BUG_FIX

			// DELETEME
			/*
			const CMotion * pkMotion = CMotionManager::instance().GetMotion(ch->GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));

			if (!pkMotion)
				sys_err("cannot find motion by race %u", ch->GetRaceNum());
			else
			{
				// 정상적 계산이라면 1000.f를 곱해야 하지만 클라이언트가 애니메이션 속도의 90%에서
				// 다음 애니메이션 블렌딩을 허용하므로 900.f를 곱한다.
				int k = (int) (pkMotion->GetDuration() / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
				ch->SetValidComboInterval(k);
				ch->SetLastComboTime(dwTime);
			}
			*/
			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
			int k = (int) (normalAttackDuration / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval(k);
			ch->SetLastComboTime(dwTime);
			// END_OF_POLYMORPH_BUG_FIX
		}
		else
		{
			// 말이 안되는 콤보가 왔다 해커일 가능성?
			//if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
			//{
			//	LogManager::instance().HackLog("Hacker", ch);
			//	sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
			//}

			// 위 코드로 인해, 폴리모프를 푸는 중에 공격 하면,
			// 가끔 핵으로 인식하는 경우가 있다.

			// 자세히 말혀면,
			// 서버에서 poly 0를 처리했지만,
			// 클라에서 그 패킷을 받기 전에, 몹을 공격. <- 즉, 몹인 상태에서 공격.
			//
			// 그러면 클라에서는 서버에 몹 상태로 공격했다는 커맨드를 보내고 (arg == 13)
			//
			// 서버에서는 race는 인간인데 공격형태는 몹인 놈이다! 라고 하여 핵체크를 했다.

			// 사실 공격 패턴에 대한 것은 클라이언트에서 판단해서 보낼 것이 아니라,
			// 서버에서 판단해야 할 것인데... 왜 이렇게 해놨을까...
			// by rtsummit
		}
	}
	else
	{
		// 말이 안되는 콤보가 왔다 해커일 가능성?
		if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
		{
			LogManager::instance().HackLog("Hacker", ch);
			sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval(300);
	}

	if (HackScalar > 0)	//@fixme429
	{
		// When riding or landing on a horse, an attack for 1.5 seconds is not considered nuclear, but has no attack power.
		if (get_dword_time() - ch->GetLastMountTime() > 1500)
			ch->IncreaseComboHackCount(1 + HackScalar);

		ch->SkipComboAttackByTime(ch->GetValidComboInterval());
	}

	return HackScalar != 0;	//@fixme429
}

void CInputMain::Move(LPCHARACTER ch, const char * data)
{
	if (!ch->CanMove())
		return;

	struct command_move * pinfo = (struct command_move *) data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

/* #ifdef ENABLE_CHECK_GHOSTMODE
	if (ch->IsPC() && ch->IsDead())
	{
		if (ch->GetGhostmodeCount() <= 10)
		{
			ch->AddGhostmodeCount();
		}
		else
		{
			LPDESC d = ch->GetDesc();
			if (d)
			{
				if (d->DelayedDisconnect(3))
				{
					LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: Ghostmode", ch);
					ch->ResetGhostmodeCount();
				}
			}
			return;
		}
	}
	else
	{
		ch->ResetGhostmodeCount();
	}
#endif
#ifdef ENABLE_CHECK_WALLHACK
	if (ch->IsPC())
	{
		if (!SECTREE_MANAGER::instance().IsMovablePosition(ch->GetMapIndex(), pinfo->lX, pinfo->lY))
		{
			if (ch->GetWallhackCount() <= 5)
			{
				ch->AddWallhackCount();
			}
			else
			{
				LPDESC d = ch->GetDesc();
				if (d)
				{
					if (d->DelayedDisconnect(3))
					{
						LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: Wallhack", ch);
						ch->ResetWallhackCount();
					}
				}
				return;
			}
		}
		else
		{
			ch->ResetWallhackCount();
		}
	}
#endif */

	//enum EMoveFuncType
	//{
	//	FUNC_WAIT,
	//	FUNC_MOVE,
	//	FUNC_ATTACK,
	//	FUNC_COMBO,
	//	FUNC_MOB_SKILL,
	//	_FUNC_SKILL,
	//	FUNC_MAX_NUM,
	//	FUNC_SKILL = 0x80,
	//};

	// 텔레포트 핵 체크

//	if (!test_server)
	{
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		// @fixme106 (changed 40 to 60)
		if (((false == ch->IsRiding() && fDist > 35) || fDist > 75) && OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
#ifdef ENABLE_HACK_TELEPORT_LOG // @warme006
			{
				const PIXEL_POSITION & warpPos = ch->GetWarpPosition();

				if (warpPos.x == 0 && warpPos.y == 0)
					LogManager::instance().HackLog("Teleport", ch); // 부정확할 수 있음
			}
#endif
			sys_log(0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
#ifdef ENABLE_CHECK_GHOSTMODE
		if (ch->IsPC() && ch->IsDead())
		{
			sys_log(0, "MOVE: %s trying to move as dead", ch->GetName());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
#endif
		//
		// 스피드핵(SPEEDHACK) Check
		//
		DWORD dwCurTime = get_dword_time();
		// 시간을 Sync하고 7초 후 부터 검사한다. (20090702 이전엔 5초였음)
		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);

		if (CheckSpeedHack)
		{
			int iDelta = (int) (pinfo->dwTime - ch->GetDesc()->GetClientTime());
			int iServerDelta = (int) (dwCurTime - ch->GetDesc()->GetClientTime());

			iDelta = (int) (dwCurTime - pinfo->dwTime);

			// 시간이 늦게간다. 일단 로그만 해둔다. 진짜 이런 사람들이 많은지 체크해야함. TODO
			if (iDelta >= 30000)
			{
				sys_log(0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
			// 1초에 20msec 빨리 가는거 까지는 이해한다.
			else if (iDelta < -(iServerDelta / 50))
			{
				sys_log(0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
		}

		//
		// 콤보핵 및 스피드핵 체크
		//
		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack(ch, pinfo->bArg, pinfo->dwTime, CheckSpeedHack); // 콤보 체크
		}
	}

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5);	// 중복 코드
		ch->ResetStopTime();				// ""

		ch->Goto(pinfo->lX, pinfo->lY);
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				const char* name = ch->GetName();
				unsigned int job = ch->GetJob();
				unsigned int group = ch->GetSkillGroup();

				char szBuf[256];
				snprintf(szBuf, sizeof(szBuf), "SKILL_HACK: name=%s, job=%d, group=%d, motion=%d", name, job, group, motion);
				LogManager::instance().HackLog(szBuf, ch->GetDesc()->GetAccountTable().login, ch->GetName(), ch->GetDesc()->GetHostName());
				sys_log(0, "%s", szBuf);

				if (test_server)
				{
					ch->GetDesc()->DelayedDisconnect(number(2, 8));
					ch->ChatPacket(CHAT_TYPE_INFO, szBuf);
				}
				else
				{
					ch->GetDesc()->DelayedDisconnect(number(150, 500));
				}
			}

			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5);	// 중복 코드
		ch->ResetStopTime();				// ""

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
#ifdef ENABLE_AFK_MODE_SYSTEM
		if (ch->IsAway())
		{
			ch->SetAway(false);
			if (ch->IsAffectFlag(AFF_AFK))
				ch->RemoveAffect(AFFECT_AFK);
		}
#endif
	}

	TPacketGCMove pack;

	pack.bHeader      = HEADER_GC_MOVE;
	pack.bFunc        = pinfo->bFunc;
	pack.bArg         = pinfo->bArg;
	pack.bRot         = pinfo->bRot;
	pack.dwVID        = ch->GetVID();
	pack.lX           = pinfo->lX;
	pack.lY           = pinfo->lY;
	pack.dwTime       = pinfo->dwTime;
	pack.dwDuration   = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);
/*
	if (pinfo->dwTime == 10653691) // 디버거 발견
	{
		if (ch->GetDesc()->DelayedDisconnect(number(15, 30)))
			LogManager::instance().HackLog("Debugger", ch);

	}
	else if (pinfo->dwTime == 10653971) // Softice 발견
	{
		if (ch->GetDesc()->DelayedDisconnect(number(15, 30)))
			LogManager::instance().HackLog("Softice", ch);
	}
*/
	/*
	sys_log(0,
			"MOVE: %s Func:%u Arg:%u Pos:%dx%d Time:%u Dist:%.1f",
			ch->GetName(),
			pinfo->bFunc,
			pinfo->bArg,
			pinfo->lX / 100,
			pinfo->lY / 100,
			pinfo->dwTime,
			fDist);
	*/
}

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
			case SKILL_GEOMPUNG:
			case SKILL_SANGONG:
			case SKILL_YEONSA:
			case SKILL_KWANKYEOK:
			case SKILL_HWAJO:
			case SKILL_GIGUNG:
			case SKILL_PABEOB:
			case SKILL_MARYUNG:
			case SKILL_TUSOK:
			case SKILL_MAHWAN:
			case SKILL_BIPABU:
			case SKILL_NOEJEON:
			case SKILL_CHAIN:
			case SKILL_HORSE_WILDATTACK_RANGE:
				if (HEADER_CG_SHOOT != type->header)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type->type);
					return;
				}
				break;
		}
	}

	switch (header)
	{
		case HEADER_CG_ATTACK:
			{
				if (NULL == ch->GetDesc())
					return;

				const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);
				ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

				LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

				if (NULL == victim || ch == victim)
					return;

				switch (victim->GetCharType())
				{
					case CHAR_TYPE_NPC:
					case CHAR_TYPE_WARP:
					case CHAR_TYPE_PET:
					case CHAR_TYPE_GOTO:
						return;
				}

#ifdef KILL_AURA_FIX
				// If the attack type is Melee
				if (packMelee->bType == 0)
				{
					// Get the difference in meter between the attacker and the victim
					// A value of 1.0f is considered a point blank hit
					const float fHittingDistance = DISTANCE_SQRT((victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100);
					// The maximum possible distance to avoid false positive is 7.0m
					// Two hands or one handed sword with Aura doesn't allow this range.
					static const float fMaximumHittingDistance = 7.0f;
					// We got one - Abort and then Log / Disconnect / Ban / Whatever?
					if (fHittingDistance > fMaximumHittingDistance)
					{
						// INSERT YOUR ACTION HERE
						return;
					}
				}
#endif
				if (packMelee->bType > 0)
				{
					if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
					{
						return;
					}
				}

				ch->Attack(victim, packMelee->bType);
			}
			break;

		case HEADER_CG_SHOOT:
			{
				const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

				ch->Shoot(packShoot->bType);
			}
			break;
	}
}

int CInputMain::SyncPosition(LPCHARACTER ch, const char * c_pcData, size_t uiBytes)
{
	const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>( c_pcData );

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
	{
		sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
		return iExtraLen;
	}

	int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

	if (iCount <= 0)
		return iExtraLen;

	static const int nCountLimit = 16;

	if( iCount > nCountLimit )
	{
		//LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );
		sys_err( "Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName() );
		//ch->GetDesc()->SetPhase(PHASE_CLOSE);
		//return -1;
		iCount = nCountLimit;
	}

	TEMP_BUFFER tbuf;
	LPBUFFER lpBuf = tbuf.getptr();

	TPacketGCSyncPosition * pHeader = (TPacketGCSyncPosition *) buffer_write_peek(lpBuf);
	buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

	const TPacketCGSyncPositionElement* e =
		reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	for (int i = 0; i < iCount; ++i, ++e)
	{
		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
			case CHAR_TYPE_NPC:
			case CHAR_TYPE_WARP:
			case CHAR_TYPE_PET:
			case CHAR_TYPE_GOTO:
				continue;
		}

		// 소유권 검사
		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT( (victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100 );
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;
		// victim과의 거리가 2500 + a 이상이면 핵으로 간주.
		//	거리 참조 : 클라이언트의 __GetSkillTargetRange, __GetBowRange 함수
		//	2500 : 스킬 proto에서 가장 사거리가 긴 스킬의 사거리, 또는 활의 사거리
		//	a = POINT_BOW_DISTANCE 값... 인데 실제로 사용하는 값인지는 잘 모르겠음. 아이템이나 포션, 스킬, 퀘스트에는 없는데...
		//		그래도 혹시나 하는 마음에 버퍼로 사용할 겸해서 1000.f 로 둠...
		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			// g_iSyncHackLimitCount번 까지는 봐줌.
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

				sys_err( "Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					e->lX, e->lY );

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}

		const float fDist = DISTANCE_SQRT( (victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100 );
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval &tvLastSyncTime = victim->GetLastSyncTime();
		timeval *tvDiff = timediff(&tvCurTime, &tvLastSyncTime);

		// SyncPosition을 악용하여 타유저를 이상한 곳으로 보내는 핵 방어하기 위하여,
		// 같은 유저를 g_lValidSyncInterval ms 이내에 다시 SyncPosition하려고 하면 핵으로 간주.
		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			// g_iSyncHackLimitCount번 까지는 봐줌.
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

				sys_err( "Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					e->lX, e->lY );

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}
		else if( fDist > 25.0f )
		{
			LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

			sys_err( "Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				   	fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				  e->lX, e->lY );

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return -1;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(e->lX, e->lY);
			buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
		}
	}

	if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
	{
		pHeader->bHeader = HEADER_GC_SYNC_POSITION;
		pHeader->wSize = buffer_size(lpBuf);

		ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
	}

	return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char * pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting * p = (TPacketCGFlyTargeting *) pcData;
	ch->FlyTarget(p->dwTargetVID, p->x, p->y, bHeader);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char * pcData)
{
	TPacketCGUseSkill * p = (TPacketCGUseSkill *) pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton * p = (TPacketCGScriptButton *) c_pData;
	sys_log(0, "QUEST ScriptButton pid %d idx %u", ch->GetPlayerID(), p->idx);

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		//퀘스트 창에서 클릭시(__SelectQuest) 여기로
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer * p = (TPacketCGScriptAnswer *) c_pData;
	sys_log(0, "QUEST ScriptAnswer pid %d answer %d", ch->GetPlayerID(), p->answer);

	if (p->answer > 250) // 다음 버튼에 대한 응답으로 온 패킷인 경우
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else // 선택 버튼을 골라서 온 패킷인 경우
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(),  p->answer);
	}
}


// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*) c_pData;
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection);
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString * p = (TPacketCGQuestInputString*) c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), msg);

	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*) c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->requestPID, (ch_wait)?ch_wait->GetName():"", p->answer);
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType) p->answer, ch->GetPlayerID());
	}
}

#ifdef ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
void CInputMain::BuyBonusChangerInSwitchBot( LPCHARACTER ch )
{
	if(ch->CountSpecifyItem(BONUS_CHANGER_ITEM_VALUE) > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO , "Possiedi ancora degli item Incanta oggetto , terminali prima di acquistarne altri. ");
		return;
	}
	
	int	iEmptyPos = ch->GetEmptyInventory(1);
	
	if (iEmptyPos < 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO , "Non hai abbastanza spazio nell'inventario.");
		return;
	}
	
	if(ch->GetGold() < BONUS_CHANGER_PRICE_IN_SWITCH_BOT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO , "Non hai abbastanza yang. (prezzo incanta bonus 2.000.000 yang) ");
		return;
	}
	
	
#ifdef FULL_YANG_OWN
	ch->ChangeGold(-BONUS_CHANGER_PRICE_IN_SWITCH_BOT );
#else
	ch->PointChange(POINT_GOLD, -BONUS_CHANGER_PRICE_IN_SWITCH_BOT, false);
#endif

	LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(BONUS_CHANGER_ITEM_VALUE, 200);
	pkNewItem->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
}

#endif


void CInputMain::Target(LPCHARACTER ch, const char * pcData)
{
	TPacketCGTarget * p = (TPacketCGTarget *) pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget;
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.dwVID = p->dwVID;
#if defined(__SHIP_DEFENSE__)
		pckTarget.bAlliance = false;
		pckTarget.iAllianceMinHP = 0;
		pckTarget.iAllianceMaxHP = 0;
#endif
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char * pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char * c_pData)
{
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin * p = (TPacketCGSafeboxCheckin *) c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

	if (p->ItemPos.IsSwitchbotPosition())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This item can not be moved here (1)");
		return;
	}

	if (pkItem->GetCell() >= INVENTORY_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 창고로 옮길 수 없는 아이템 입니다."));
	    return;
	}

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 옮길 수 없는 위치입니다."));
		return;
	}

	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi effettuare questa operazione."));
		return;
	}

	if( IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX) )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi effettuare questa operazione."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi effettuare questa operazione."));
		return;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pkItem->IsEquipped())
	{
		int iWearCell = pkItem->FindEquipCell(ch);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !ch->UnequipItem(costumeWeapon))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"Non puoi equipaggiare il costume. Non hai abbastanza spazio."));
				return;
			}
		}
	}
#endif

	// @fixme140 BEGIN
	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"벨트 인벤토리에 아이템이 존재하면 해제할 수 없습니다."));
		return;
	}
	// @fixme140 END

	pkItem->RemoveFromCharacter();
	if (!pkItem->IsDragonSoul())
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, 255);
	pkSafebox->Add(p->bSafePos, pkItem);

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(ch->GetLanguage()), pkItem->GetCount());
	LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX PUT", szHint);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall)
{
	TPacketCGSafeboxCheckout * p = (TPacketCGSafeboxCheckout *) c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	// 아이템 몰에서 인벤으로 옮기는 부분에서 용혼석 특수 처리
	// (몰에서 만드는 아이템은 item_proto에 정의된대로 속성이 붙기 때문에,
	//  용혼석의 경우, 이 처리를 하지 않으면 속성이 하나도 붙지 않게 된다.)
	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 옮길 수 없는 위치입니다."));
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<창고> 옮길 수 없는 위치입니다."));
				return ;
			}
			DestPos = TItemPos (DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
#ifdef __SPECIAL_STORAGE_SYSTEM__
	else if (pkItem->IsSpecialStorageItem())
	{
		char szTemp[256];

		snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), pkItem->GetCount());

		if (pkItem->GetSpecialWindowType() != p->ItemPos.window_type && INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"L'oggetto %s%s non puo' essere riposto qui."), pkItem->GetName(ch->GetLanguage()), (pkItem->GetCount() > 1 ? szTemp : " "));
			return;
		}

		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"AI ¾ÆAIAUAº º§Æ® AIº￥Aa¸®·I ¿A±æ ¼o ¾ø½A´I´U."));
			return;
		}

		int iCell = ch->GetEmptySpecialStorageSlot(pkItem);

		if (iCell < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<A￠°i> ¿A±æ ¼o ¾ø´A A§A¡AO´I´U."));
			return;
		}

		pkSafebox->Remove(p->bSafePos);

#ifdef __HIGHLIGHT_SYSTEM__
		pkItem->AddToCharacter(ch, p->ItemPos, false);
#else
		pkItem->AddToCharacter(ch, p->ItemPos);
#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
#endif
	else
	{
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<A￠°i> ¿A±æ ¼o ¾ø´A A§A¡AO´I´U."));
			return;
		}

		char szTemp[256];

		snprintf(szTemp, sizeof(szTemp), LC_TEXT(" x%d "), pkItem->GetCount());

#ifdef __SPECIAL_STORAGE_SYSTEM__
		if (p->ItemPos.window_type >= SKILLBOOK_INVENTORY && p->ItemPos.window_type <= GENERAL_INVENTORY)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"L'oggetto %s%s non puo' essere riposto qui."), pkItem->GetName(ch->GetLanguage()), (pkItem->GetCount() > 1 ? szTemp : " "));
			return;
		}
#endif
		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"이 아이템은 벨트 인벤토리로 옮길 수 없습니다."));
			return;
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(ch->GetLanguage()), pkItem->GetCount());
	if (bMall)
		LogManager::instance().ItemLog(ch, pkItem, "MALL GET", szHint);
	else
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX GET", szHint);
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char * data)
{
	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell, pinfo->count);
}

#if defined(BL_REMOTE_SHOP)
static bool RemoteShopRestrictedMap(long lMapIndex)
{
	switch (lMapIndex)
	{
	case 113:
		return false;
	}
	return true;
}
void CInputMain::RemoteShop(LPCHARACTER ch, const char* data)
{
	const TPacketCGRemoteShop* p = reinterpret_cast<const TPacketCGRemoteShop*>(data);

	if (RemoteShopRestrictedMap(ch->GetMapIndex()) == false)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Negozio> Non puoi aprirlo in questa mappa.");
		return;
	}

	//PREVENT_TRADE_WINDOW
	if (ch->IsOpenSafebox() || ch->GetExchange() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen() || ch->GetOfflineShopOwner() || ch->GetMailBox())
		return;
	//END_PREVENT_TRADE_WINDOW

	const DWORD dwCurrentTime = get_dword_time(), dwLimitTime = ch->GetLastRemoteTime() + 15000; // 15 sec.
	if (dwCurrentTime < dwLimitTime) {
		ch->ChatPacket(CHAT_TYPE_INFO, "<Negozio> devi attendere %u sec.", MAX(1, (dwLimitTime - dwCurrentTime) / 1000));
		return;
	}

	static const/*expr*/ DWORD _arrShopVnum[] = { 1, 5, 4, 38, 40 };
	if (p->shop_index >= _countof(_arrShopVnum)) {
		sys_err("RemoteShop player(%s) unknown index(%d)", ch->GetName(), p->shop_index);
		return;
	}

	const DWORD dwShopVnum = _arrShopVnum[p->shop_index];
	LPSHOP pkShop = CShopManager::instance().Get(dwShopVnum);
	if (pkShop == NULL)
		return;

	pkShop->AddGuest(ch, 0, false);
	ch->SetShopOwner(ch);
	ch->SetLastRemoteTime(get_dword_time());
	sys_log(0, "SHOP: START: %s", ch->GetName());
}
#endif

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char * c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"대련장에서 사용하실 수 없습니다."));
		return;
	}
#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "Non puoi farlo nell'arena.."));
		return;
	}
#endif
	TPacketCGPartyInvite * p = (TPacketCGPartyInvite*) c_pData;

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc())
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}
	#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().CheckMessengerList(ch->GetName(), pInvitee->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s blokkk"), pInvitee->GetName());
		return;
	}
	#endif

#ifdef __REMOVE_PARTY_IN_SPECIFIC_MAP
	if(!ch->CanEnterParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't in party in this map!");
		return;
	}
#endif

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char * c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"대련장에서 사용하실 수 없습니다."));
		return;
	}

#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "Non puoi farlo nell'arena.."));
		return;
	}
#endif
#ifdef __REMOVE_PARTY_IN_SPECIFIC_MAP
	if(!ch->CanEnterParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't in party in this map!");
		return;
	}
#endif

	TPacketCGPartyInviteAnswer * p = (TPacketCGPartyInviteAnswer*) c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	// pInviter 가 ch 에게 파티 요청을 했었다.

	if (!pInviter)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 파티요청을 한 캐릭터를 찾을수 없습니다."));
	else if (!p->accept)
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*) c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 리더만 변경할 수 있습니다."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 상태를 변경하려는 사람이 파티원이 아닙니다."));
		return;
	}

	DWORD pid = p->pid;
	sys_log(0, "PARTY SetRole pid %d to role %d state %s", pid, p->byRole, p->flag ? "on" : "off");

	switch (p->byRole)
	{
		case PARTY_ROLE_NORMAL:
			break;

		case PARTY_ROLE_ATTACKER:
		case PARTY_ROLE_TANKER:
		case PARTY_ROLE_BUFFER:
		case PARTY_ROLE_SKILL_MASTER:
		case PARTY_ROLE_HASTE:
		case PARTY_ROLE_DEFENDER:
			if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
			{
				TPacketPartyStateChange pack;
				pack.dwLeaderPID = ch->GetPlayerID();
				pack.dwPID = p->pid;
				pack.bRole = p->byRole;
				pack.bFlag = p->flag;
				db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
			}
			/* else
			   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 어태커 설정에 실패하였습니다.")); */
			break;

		default:
			sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
			break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"대련장에서 사용하실 수 없습니다."));
		return;
	}

#ifdef ENABLE_DECORUM
	if (ch->GetDecorumArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(), "Non puoi farlo nell'arena.."));
		return;
	}
#endif

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 던전 안에서는 파티에서 추방할 수 없습니다."));
		return;
	}

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*) c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 던젼내에서는 파티원을 추방할 수 없습니다."));
		}
		else
		{
			// 적룡성에서 파티장이 던젼 밖에서 파티 해산 못하게 막자
			if(pParty->IsPartyInDungeon(351))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티>던전 안에 파티원이 있어 파티를 해산 할 수 없습니다."));
				return;
			}

			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(B->GetLanguage(),"<파티> 파티에서 추방당하셨습니다."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 파티에서 나가셨습니다."));
					//pParty->SendPartyRemoveOneToAll(ch);
					pParty->Quit(ch->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
					//CPartyManager::instance().SetPartyMember(ch->GetPlayerID(), NULL);
				}
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 다른 파티원을 탈퇴시킬 수 없습니다."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*) c_pData;

	if (ch->GetGold() < 200000)
		return;

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
			CGuildManager::instance().GetDisbandDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 해산한 후 %d일 이내에는 길드를 만들 수 없습니다."),
				quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
			CGuildManager::instance().GetWithdrawDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 탈퇴한 후 %d일 이내에는 길드를 만들 수 없습니다."),
				quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"적합하지 않은 길드 이름 입니다."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> [%s] 길드가 생성되었습니다."), cp.name);

		int GuildCreateFee = 200000;

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		DBManager::instance().SendMoneyLog(MONEY_LOG_GUILD, ch->GetPlayerID(), -GuildCreateFee);

		char Log[128];
		snprintf(Log, sizeof(Log), "GUILD_NAME %s MASTER %s", cp.name, ch->GetName());
		LogManager::instance().CharLog(ch, 0, "MAKE_GUILD", Log);

		ch->RemoveSpecifyItem(GUILD_CREATE_ITEM_VNUM, 1);
		//ch->SendGuildName(dwGuildID);
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드 생성에 실패하였습니다."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*) c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 파티 기술은 파티장만 사용할 수 있습니다."));
		return;
	}

	switch (p->bySkillIndex)
	{
		case PARTY_SKILL_HEAL:
			ch->GetParty()->HealParty();
			break;
		case PARTY_SKILL_WARP:
			{
				LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
				if (pch)
					ch->GetParty()->SummonToLeader(pch->GetPlayerID());
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<파티> 소환하려는 대상을 찾을 수 없습니다."));
			}
			break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGPartyParameter * p = (TPacketCGPartyParameter *) c_pData;

	if (ch->GetParty())
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

#ifdef __INGAME_WIKI__
void CInputMain::RecvWikiPacket(LPCHARACTER ch, const char * c_pData)
{
	if (!ch || (ch && !ch->GetDesc()))
		return;
	
	if (!c_pData)
		return;
	
	InGameWiki::TCGWikiPacket * p = (InGameWiki::TCGWikiPacket*)c_pData;
	
	InGameWiki::TGCWikiPacket pack;
	pack.set_data_type(!p->is_mob ? InGameWiki::LOAD_WIKI_ITEM : InGameWiki::LOAD_WIKI_MOB);
	pack.increment_data_size(WORD(sizeof(InGameWiki::TGCWikiPacket)));
	
	if (pack.is_data_type(InGameWiki::LOAD_WIKI_ITEM))
	{
		const std::vector<CommonWikiData::TWikiItemOriginInfo>& originVec = ITEM_MANAGER::Instance().GetItemOrigin(p->vnum);
		const std::vector<CSpecialItemGroup::CSpecialItemInfo> _gV = ITEM_MANAGER::instance().GetWikiChestInfo(p->vnum);
		const std::vector<CommonWikiData::TWikiRefineInfo> _rV = ITEM_MANAGER::instance().GetWikiRefineInfo(p->vnum);
		const CommonWikiData::TWikiInfoTable* _wif = ITEM_MANAGER::instance().GetItemWikiInfo(p->vnum);
		
		if (!_wif)
			return;
		
		const size_t origin_size = originVec.size();
		const size_t chest_info_count = _wif->chest_info_count;
		const size_t refine_infos_count = _wif->refine_infos_count;
		const size_t buf_data_dize = sizeof(InGameWiki::TGCItemWikiPacket) +
								(origin_size * sizeof(CommonWikiData::TWikiItemOriginInfo)) +
								(chest_info_count * sizeof(CommonWikiData::TWikiChestInfo)) +
								(refine_infos_count * sizeof(CommonWikiData::TWikiRefineInfo));
		
		if (chest_info_count != _gV.size()) {
			sys_err("Item Vnum : %d || ERROR TYPE -> 1", p->vnum);
			return;
		}
		
		if (refine_infos_count != _rV.size()) {
			sys_err("Item Vnum : %d || ERROR TYPE -> 2", p->vnum);
			return;
		}
		
		pack.increment_data_size(WORD(buf_data_dize));
		
		TEMP_BUFFER buf;
		buf.write(&pack, sizeof(InGameWiki::TGCWikiPacket));
		
		InGameWiki::TGCItemWikiPacket data_packet;
		data_packet.mutable_wiki_info(*_wif);
		data_packet.set_origin_infos_count(origin_size);
		data_packet.set_vnum(p->vnum);
		data_packet.set_ret_id(p->ret_id);
		buf.write(&data_packet, sizeof(data_packet));
		
		{
			if (origin_size)
				for (int idx = 0; idx < (int)origin_size; ++idx)
					buf.write(&(originVec[idx]), sizeof(CommonWikiData::TWikiItemOriginInfo));
			
			if (chest_info_count > 0) {
				for (int idx = 0; idx < (int)chest_info_count; ++idx) {
					CommonWikiData::TWikiChestInfo write_struct(_gV[idx].vnum, _gV[idx].count);
					buf.write(&write_struct, sizeof(CommonWikiData::TWikiChestInfo));
				}
			}
			
			if (refine_infos_count > 0)
				for (int idx = 0; idx < (int)refine_infos_count; ++idx)
					buf.write(&(_rV[idx]), sizeof(CommonWikiData::TWikiRefineInfo));
		}
		
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
	else
	{
		CMobManager::TMobWikiInfoVector& mobVec = CMobManager::instance().GetMobWikiInfo(p->vnum);
		const size_t _mobVec_size = mobVec.size();
		
		if (!_mobVec_size) {
			if (test_server)
				sys_log(0, "Mob Vnum: %d : || LOG TYPE -> 1", p->vnum);
			return;
		}
		
		const size_t buf_data_dize = (sizeof(InGameWiki::TGCMobWikiPacket) + (_mobVec_size * sizeof(CommonWikiData::TWikiMobDropInfo)));
		pack.increment_data_size(WORD(buf_data_dize));
		
		TEMP_BUFFER buf;
		buf.write(&pack, sizeof(InGameWiki::TGCWikiPacket));
		
		InGameWiki::TGCMobWikiPacket data_packet;
		data_packet.set_drop_info_count(_mobVec_size);
		data_packet.set_vnum(p->vnum);
		data_packet.set_ret_id(p->ret_id);
		buf.write(&data_packet, sizeof(InGameWiki::TGCMobWikiPacket));
		
		{
			if (_mobVec_size) {
				for (int idx = 0; idx < (int)_mobVec_size; ++idx) {
					CommonWikiData::TWikiMobDropInfo write_struct(mobVec[idx].vnum, mobVec[idx].count);
					buf.write(&write_struct, sizeof(CommonWikiData::TWikiMobDropInfo));
				}
			}
		}
		
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}
#endif

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:				return sizeof(int);
		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:				return sizeof(int);
		case GUILD_SUBHEADER_CG_ADD_MEMBER:					return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:			return 10;
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:		return sizeof(BYTE) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_OFFER:						return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHARGE_GSP:					return sizeof(int);
		case GUILD_SUBHEADER_CG_POST_COMMENT:				return 1;
		case GUILD_SUBHEADER_CG_DELETE_COMMENT:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:			return 0;
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_USE_SKILL:					return sizeof(TPacketCGGuildUseSkill);
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:		return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드에 속해있지 않습니다."));
			return SubPacketLen;
		}
	}

	switch (SubHeader)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
			{
				// by mhh : 길드자금은 당분간 넣을 수 없다.
				return SubPacketLen;

				const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 잘못된 금액입니다."));
					return SubPacketLen;
				}

				if (ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 가지고 있는 돈이 부족합니다."));
					return SubPacketLen;
				}

				pGuild->RequestDepositMoney(ch, gold);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
			{
				// by mhh : 길드자금은 당분간 뺄 수 없다.
				return SubPacketLen;

				const int gold = MIN(*reinterpret_cast<const int*>(c_pData), 500000);

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 잘못된 금액입니다."));
					return SubPacketLen;
				}

				pGuild->RequestWithdrawMoney(ch, gold);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_ADD_MEMBER:
			{
				const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
				LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);
				#ifdef ENABLE_MESSENGER_BLOCK
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), newmember->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"%s blokkk"), newmember->GetName());
					return SubPacketLen;
				}
				#endif

				if (!newmember)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 그러한 사람을 찾을 수 없습니다."));
					return SubPacketLen;
				}

				// @fixme145 BEGIN (+newmember ispc check)
				if (!ch->IsPC() || !newmember->IsPC())
					return SubPacketLen;
				// @fixme145 END

				pGuild->Invite(ch, newmember);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
			{
				if (pGuild->UnderAnyWar() != 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드전 중에는 길드원을 탈퇴시킬 수 없습니다."));
					return SubPacketLen;
				}

				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

				if (member)
				{
					if (member->GetGuild() != pGuild)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 상대방이 같은 길드가 아닙니다."));
						return SubPacketLen;
					}

					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
						return SubPacketLen;
					}

					member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
					pGuild->RequestRemoveMember(member->GetPlayerID());

					if (g_bGuildInviteLimit)
					{
						DBManager::instance().Query("REPLACE INTO guild_invite_limit VALUES(%d, %d)", pGuild->GetID(), get_global_time());
					}
				}
				else
				{
					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
						return SubPacketLen;
					}

					if (pGuild->RequestRemoveMember(pid))
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드원을 강제 탈퇴 시켰습니다."));
					else
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 그러한 사람을 찾을 수 없습니다."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
			{
				char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
				strlcpy(gradename, c_pData + 1, sizeof(gradename));

				const TGuildMember * m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 직위 이름을 변경할 권한이 없습니다."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드장의 직위 이름은 변경할 수 없습니다."));
				}
				else if (!check_name(gradename))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 적합하지 않은 직위 이름 입니다."));
				}
				else
				{
					pGuild->ChangeGradeName(*c_pData, gradename);
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
			{
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 직위 권한을 변경할 권한이 없습니다."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드장의 권한은 변경할 수 없습니다."));
				}
				else
				{
					pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_OFFER:
			{
				DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

				if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드가 이미 최고 레벨입니다."));
				}
				else
				{
					offer /= 100;
					offer *= 100;

					if (pGuild->OfferExp(ch, offer))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> %u의 경험치를 투자하였습니다."), offer);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 경험치 투자에 실패하였습니다."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHARGE_GSP:
			{
				const int offer = *reinterpret_cast<const int*>(c_pData);
				const int gold = offer * 100;

				if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 돈이 부족합니다."));
					return SubPacketLen;
				}

				if (!pGuild->ChargeSP(ch, offer))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 용신력 회복에 실패하였습니다."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_POST_COMMENT:
			{
				const size_t length = *c_pData;

				if (length > GUILD_COMMENT_MAX_LEN)
				{
					// 잘못된 길이.. 끊어주자.
					sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
					ch->GetDesc()->SetPhase(PHASE_CLOSE);
					return -1;
				}

				if (uiBytes < 1 + length)
					return -1;

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 공지글을 작성할 권한이 없습니다."));
				}
				else
				{
					std::string str(c_pData + 1, length);
					pGuild->AddComment(ch, str);
				}

				return (1 + length);
			}

		case GUILD_SUBHEADER_CG_DELETE_COMMENT:
			{
				const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

				pGuild->DeleteComment(ch, comment_id);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
			pGuild->RefreshComment(ch);
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE grade = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 직위를 변경할 권한이 없습니다."));
				else if (ch->GetPlayerID() == pid)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드장의 직위는 변경할 수 없습니다."));
				else if (grade == 1)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 길드장으로 직위를 변경할 수 없습니다."));
				else
					pGuild->ChangeMemberGrade(pid, grade);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_USE_SKILL:
			{
				const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);
				pGuild->UseSkill(p->dwVnum, ch, p->dwPID);

			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE is_general = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 장군을 지정할 권한이 없습니다."));
				}
				else
				{
					if (!pGuild->ChangeMemberGeneral(pid, is_general))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"<길드> 더이상 장수를 지정할 수 없습니다."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
			{
				const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE accept = *(c_pData + sizeof(DWORD));

				CGuild * g = CGuildManager::instance().FindGuild(guild_id);

				if (g)
				{
					if (accept)
						g->InviteAccept(ch);
					else
						g->InviteDeny(ch->GetPlayerID());
				}
			}
			return SubPacketLen;

	}

	return 0;
}

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;
	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

#if defined(__BL_MAILBOX__)
void CInputMain::MailboxWrite(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketCGMailboxWrite*>(c_pData);
	if (p == nullptr)
		return;
	
	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

	mail->Write(p->szName, p->szTitle, p->szMessage, p->pos, p->iYang, p->iWon);
}

void CInputMain::MailboxConfirm(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketCGMailboxWriteConfirm*>(c_pData);
	if (p == nullptr)
		return;
	
	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

	mail->CheckPlayer(p->szName);
}

void CInputMain::MailboxProcess(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketMailboxProcess*>(c_pData);
	if (p == nullptr)
		return;
	
	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

	switch (p->bSubHeader)
	{
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_CLOSE:
		ch->SetMailBox(nullptr);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_DELETE:
		mail->DeleteMail(p->bArg1, false);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ALL_DELETE:
		mail->DeleteAllMails();
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_GET_ITEMS:
		mail->GetItem(p->bArg1, false);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ALL_GET_ITEMS:
		mail->GetAllItems();
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ADD_DATA:
		mail->AddData(p->bArg1, p->bArg2);
		break;
	default:
		sys_err("CInputMain::MailboxProcess Unknown SubHeader (ch: %s) (%d)", ch->GetName(), p->bSubHeader);
		break;
	}
}
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
std::string CInputMain::GetLanguageFlag(const char* c_pszName)
{
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT `language` FROM account.account INNER JOIN player.player ON player.player.account_id = account.account.id WHERE player.player.`name` = '%s'", c_pszName));
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	
	if (pMsg->Get()->uiNumRows)
	{
		std::string c_pszLanguage = row[0];
		return c_pszLanguage.c_str();
	}

	return "";
}

std::string CInputMain::ConvertEmpireIndexToStr(int empireIndex)
{
	#define EMPIRE_RED		1
	#define EMPIRE_YELLOW	2
	#define EMPIRE_BLUE		3

	switch (empireIndex)
	{
		case EMPIRE_RED:
			return "a";
		case EMPIRE_YELLOW:
			return "b";
		case EMPIRE_BLUE:
			return "c";
		default:
			return "";
	}
}

std::string CInputMain::GetEmpireFlag(const char* c_pszName)
{
	char newName[CHARACTER_NAME_MAX_LEN+1];
	DBManager::instance().EscapeString(newName, sizeof(newName), c_pszName, sizeof(c_pszName));

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT `empire` FROM player.player_index INNER JOIN player.player ON player.player.account_id = player.player_index.id WHERE player.player.`name` = '%s'", newName));
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	int iEmpireIndex = 0;
	
	if (pMsg->Get()->uiNumRows)
	{
		str_to_number(iEmpireIndex, row[0]);
		return ConvertEmpireIndexToStr(iEmpireIndex).c_str();
	}

	return "";
}

void CInputMain::RequestFlag(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRequestFlag* p = reinterpret_cast<const TPacketCGRequestFlag*>(c_pData);
	
	if (!ch)
		return;
	
	const char* c_pszName = p->name;
	std::string c_pszLanguage = ch->GetLang();
	std::string c_pszEmpire = GetEmpireFlag(c_pszName).c_str();

	if (c_pszLanguage != "" && c_pszEmpire != "")
		ch->ChatPacket(CHAT_TYPE_COMMAND, "BINARY_SET_LANG_AND_EMPIRE_FLAG %s %s %s", c_pszName, c_pszLanguage.c_str(), c_pszEmpire.c_str());
}
#endif

void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*) c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"아이템을 건네줄 수 없습니다."));
}

void CInputMain::Hack(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGHack * p = (TPacketCGHack *) c_pData;
#ifdef ENABLE_CSHIELD
	if (!p || !p->szBuf || !strlen(p->szBuf))
		return;
	
	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	char __escape_string[1024];
	DBManager::instance().EscapeString(__escape_string, sizeof(__escape_string), buf, strlen(p->szBuf));

	LogManager::instance().HackLog(__escape_string, ch);
#else
	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);
#endif

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

int CInputMain::MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes)
{	
	TPacketCGMyShop * p = (TPacketCGMyShop *) c_pData;
	
	#ifndef FULL_YANG 
	int iExtraLen = p->bCount * (sizeof(DWORD) + sizeof(BYTE) + sizeof(DWORD) + sizeof(TItemPos) + sizeof(BYTE) + sizeof(unsigned short) + sizeof(unsigned short)); //1 x 13
	#else
	int iExtraLen = p->bCount * (sizeof(DWORD) + sizeof(BYTE) + sizeof(long long) + sizeof(TItemPos) + sizeof(BYTE) + sizeof(unsigned short) + sizeof(unsigned short)); //1 x 13		
	#endif
	
#ifdef SHOP_BLOCK_GAME99
	if (g_bChannel >= 99 && ch->GetGMLevel() <= 0) {
		ch->ChatPacket(CHAT_TYPE_INFO, ( "SHOP_OX_ERROR"));
		return (iExtraLen);
	}
#endif

#ifdef SHOP_GM_PRIVILEGES
	if (GM_PLAYER < ch->GetGMLevel() && SHOP_GM_PRIVILEGES >ch->GetGMLevel()) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"SHOP_PRIV_ERROR"));
		return (iExtraLen);
	}
#endif


	if(uiBytes < sizeof(TPacketCGMyShop) + iExtraLen) //36 + 13
	{
		sys_err("Number of Bytes i know :  %u ,   number of bytes remain :  %u . (TPacketCGMyShop size : %u , iExtraLen size %u ) ",uiBytes , sizeof(TPacketCGMyShop) + iExtraLen , sizeof(TPacketCGMyShop) , iExtraLen ) ;
		return -1;
	}
	
	#ifndef FULL_YANG
 	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"소유 돈이 20억냥을 넘어 거래를 핼수가 없습니다."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}
	#endif
	
	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

#ifdef OFFLINE_SHOP
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->GetOfflineShopOwner() || ch->GetMailBox())
#else
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen())
#endif
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT_LANGUAGE(ch->GetLanguage(),"다른 거래중일경우 개인상점을 열수가 없습니다."));
		return (iExtraLen);
	}

	sys_log(0, "MyShop count %d", p->bCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable *)(c_pData + sizeof(TPacketCGMyShop)), p->bCount);

	
	return (iExtraLen);
}

#ifdef OFFLINE_SHOP
int CInputMain::MyOfflineShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes)
{
	TPacketCGMyOfflineShop * p = (TPacketCGMyOfflineShop *)c_pData;
	int iExtraLen = p->bCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyOfflineShop) + iExtraLen)
		return -1;

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->GetOfflineShopOwner() || ch->GetMailBox())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중일경우 개인상점을 열수가 없습니다."));
		return (iExtraLen);
	}

	sys_log(0, "MyOfflineShop count %d", p->bCount);
	ch->OpenMyOfflineShop(p->szSign, (TShopItemTable *)(c_pData + sizeof(TPacketCGMyOfflineShop)), p->bCount, p->bTime);
	return (iExtraLen);
}
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
void CInputMain::ShopSearchInfo(LPCHARACTER ch, const char * data, bool bNameOnly)
{
	TPacketCGShopSearchInfo * p = (TPacketCGShopSearchInfo *)data;

	if (!ch)
		return;

	ch->ShopSearchInfo(bNameOnly, p->bItemType, p->bItemSubType, p->lMinGold, p->lMaxGold, p->bMinCheque, p->bMaxCheque, p->szItemName);
}

void CInputMain::ShopSearchBuyItem(LPCHARACTER ch, const char * data)
{
	TPacketCGShopSearchBuyItem * p = (TPacketCGShopSearchBuyItem *)data;

	if (!ch)
		return;

	ch->ShopSearchBuyItem(p->dwShopVID, p->bItemPos);
}
#endif

#ifdef CHANGELOOK_SYSTEM
void CInputMain::ChangeLook(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketChangeLook * sPacket = (TPacketChangeLook*)c_pData;
	switch (sPacket->subheader)
	{
	case CL_SUBHEADER_CLOSE:
	{
		pkChar->ChangeLookWindow(false);
	}
	break;
	case CL_SUBHEADER_ADD:
	{
		pkChar->AddClMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case CL_SUBHEADER_REMOVE:
	{
		pkChar->RemoveClMaterial(sPacket->bPos);
	}
	break;
	case CL_SUBHEADER_REFINE:
	{
		pkChar->RefineClMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

#ifdef OFFLINE_SHOP
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen() || ch->GetOfflineShopOwner() || ch->GetMailBox())
#else
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen())
#endif
	{
		ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
		ch->ClearRefineMode();
		return;
	}

	if (p->type == 255)
	{
		// DoRefine Cancel
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= INVENTORY_MAX_NUM)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}

	ch->SetRefineTime();

	if (p->type == REFINE_TYPE_NORMAL)
	{
		sys_log (0, "refine_type_noraml");
		ch->DoRefine(item);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_HYUNIRON || p->type == REFINE_TYPE_MUSIN || p->type == REFINE_TYPE_BDRAGON
		)
	{
		sys_log (0, "refine_type_scroll, ...");
		ch->DoRefineWithScroll(item);
	}
	else if (p->type == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem(p->pos);

		if (NULL != item)
		{
			if (500 <= item->GetRefineSet())
			{
				LogManager::instance().HackLog("DEVIL_TOWER_REFINE_HACK", ch);
			}
			else
			{
				if (ch->GetQuestFlag("deviltower_zone.can_refine"))
				{
					ch->DoRefine(item, true);
					ch->SetQuestFlag("deviltower_zone.can_refine", 0);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "사귀 타워 완료 보상은 한번까지 사용가능합니다.");
				}
			}
		}
	}

	ch->ClearRefineMode();
}

#ifdef SYSTEM_PDA
void CInputMain::SoulStoneUse(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGSoulStoneUse* p = (TPacketCGSoulStoneUse*)c_pData;

	// check general data
	const DWORD* c_adwAvailSkillVnums = ch->GetUsableSkillList();
	if (!c_adwAvailSkillVnums)
		return;

	bool bFoundVnum = false;
	for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
	{
		if (!c_adwAvailSkillVnums[i])
			continue;

		if (c_adwAvailSkillVnums[i] == p->skillIndex)
		{
			bFoundVnum = true;
			break;
		}
	}
	if (!bFoundVnum)
		return;

	if (ch->GetSkillMasterType(p->skillIndex) != SKILL_GRAND_MASTER)
		return;

	if (!p->Cell.IsDefaultInventoryPosition())
		return;

	// check item
	LPITEM pkItem = ch->GetInventoryItem(p->Cell.cell);
	if (!pkItem || !CItemVnumHelper::IsSoulStone(pkItem->GetVnum()))
		return;

	if (pkItem->IsExchanging())
		return;

	if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		return;
	
	// use soulstones
	BYTE bCount = p->all ? pkItem->GetCount() : 1;
	BYTE bUseCount = 0;
	BYTE bSuccessCount = 0;

	for (int i = 0; i < bCount; ++i)
	{
		++bUseCount;

		if (ch->LearnGrandMasterSkill(p->skillIndex))
		{
			++bSuccessCount;
			if (ch->GetSkillMasterType(p->skillIndex) > SKILL_GRAND_MASTER)
				break;
		}
	}

	pkItem->SetCount(pkItem->GetCount() - bUseCount);

	if (bSuccessCount == 0)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Egitim basarisiz oldu."));
	else if (ch->GetSkillMasterType(p->skillIndex) > SKILL_GRAND_MASTER)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Grandmaster seviyesinde basarili oldunuz."));
	else if (bCount == 1)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Egitim basarili oldu."));
	else if (bCount == bSuccessCount)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Tum ruh taslari basariyla kullanildi."));
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d/%d ruh taslari basariyla kullanildi."), bSuccessCount, bCount);
}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;
	TPacketAcce * sPacket = (TPacketAcce*)c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#ifdef ANTY_WAIT_HACK
extern std::map<BYTE, std::string> AntyWHSums;
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
void CInputMain::CubeRenewalSend(LPCHARACTER ch, const char* data)
{
	struct packet_send_cube_renewal * pinfo = (struct packet_send_cube_renewal *) data;
	switch (pinfo->subheader)
	{
	case CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM:
	{

		int index_item = pinfo->index_item;
		int count_item = pinfo->count_item;
		int index_item_improve = pinfo->index_item_improve;

		Cube_Make(ch, index_item, count_item, index_item_improve);
	}
	break;

	case CUBE_RENEWAL_SUB_HEADER_CLOSE:
	{
		Cube_close(ch);
	}
	break;
	}
}
#endif

#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
void CInputMain::Bonus67NewSend(LPCHARACTER ch, const char * data)
{

	struct packet_send_67_bonus_system * pinfo = (struct packet_send_67_bonus_system *) data;

	int subheader = pinfo->subheader;

	if (subheader < 0 and subheader > 2)
		return;

	switch (subheader)
	{
	case BONUS_67_NEW_SUB_HEADER_CLOSE:
	{
		if (!ch->IsOpenCombSkillBook())
			return;

		ch->SetOpenCombSkillBook(false);

	}
	break;

	case BONUS_67_NEW_SUB_HEADER_OPEN:
	{
		if (ch->IsOpenCombSkillBook())
			return;
		ch->SetOpenCombSkillBook(true);
	}
	break;

	case BONUS_67_NEW_SUB_HEADER_COMB_SKILLBOOK:
	{
		C67BonusNew::instance().CombiStart(ch, pinfo->cell);
	}
	break;

	case BONUS_67_NEW_SUB_HEADER_GET_FRAGMENT:
	{
		C67BonusNew::instance().ChechFragment(ch, pinfo->regist_slot);
	}
	break;

	case BONUS_67_NEW_SUB_HEADER_ADD:
	{
		C67BonusNew::instance().AddAttr(ch, pinfo->regist_slot, pinfo->count_fragment, pinfo->cell_additive, pinfo->count_additive);
	}

	break;
	}
}
#endif

#ifdef ENABLE_HUNTING_SYSTEM
int CInputMain::ReciveHuntingAction(LPCHARACTER ch, const char* c_pData)
{
	TPacketGCHuntingAction* p = (TPacketGCHuntingAction*)c_pData;

	switch (p->bAction)
	{
		// Open Windows
		case 1:
			
			ch->OpenHuntingWindowMain();
			break;

		// Select Type
		case 2:
			if (ch->GetQuestFlag("hunting_system.is_active") == -1) {
				
				if (ch->GetLevel() < ch->GetQuestFlag("hunting_system.level"))
					return 0;
				
				ch->SetQuestFlag("hunting_system.is_active", 1);
				ch->SetQuestFlag("hunting_system.type", p->dValue);
				ch->SetQuestFlag("hunting_system.count", 0);
				
				ch->OpenHuntingWindowMain();

			} 
			else 
				ch->ChatPacket(CHAT_TYPE_INFO, "Es ist noch eine Jagdmission offen.");
			break;
		
		// Open Reward Window
		case 3:
			ch->OpenHuntingWindowReward();
			break;
			
		// Recive Reward
		case 4:
			ch->ReciveHuntingRewards();
			break;

		default:
			break;
	}

	return 0;
}
#endif

#ifdef ENABLE_CSHIELD
void CInputMain::CShieldReceive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGCShield* p = (TPacketCGCShield*)c_pData;

	if (!ch)
		return;

	if (!c_pData)
		return;

	if (strlen(p->key) < 64)
		return;

	ch->GetCShield()->SetKeyStored(p->key);
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	int iExtraLen = 0;

	if (test_server && bHeader != HEADER_CG_MOVE)
		sys_log(0, "CInputMain::Analyze() ==> Header [%d] ", bHeader);

#ifdef ENABLE_AFK_MODE_SYSTEM
	if(bHeader != HEADER_CG_PONG && bHeader != HEADER_CG_TIME_SYNC)
	{
		ch->SetLastPacketTime(time(0));
		if(ch->IsAway())
		{
			ch->RemoveAffect(AFFECT_AFK);
			ch->SetAway(false);
		}
	}
#endif


	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;
			
#ifdef ANTY_WAIT_HACK
		case HEADER_CG_ANTY_WAIT_HACK:
		{
			LPCHARACTER ch;

			if (!(ch = d->GetCharacter()))
			{
				sys_err("no character on desc");
				return 0;
			}
			TPacketCGAntyWH* p = reinterpret_cast <TPacketCGAntyWH*>((void*)c_pData);

			if (AntyWHSums.find(ch->GetRealJob()) != AntyWHSums.end())
			{

				sys_log(1, "Checking file %s hash %s", p->file, AntyWHSums[ch->GetRealJob()].c_str());
				ch->SetLastWHRegex(p->file);
				if (AntyWHSums[ch->GetRealJob()] == p->hash)
					ch->SetWHPass();
			}
	}
	break;
#endif

		case HEADER_CG_CHAT:
			if (test_server)
			{
				char* pBuf = (char*)c_pData;
				sys_log(0, "%s", pBuf + sizeof(TPacketCGChat));
			}

			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_MOVE:
			Move(ch, c_pData);
			// @fixme103 (removed CheckClientVersion since useless in here)
			break;

		case HEADER_CG_CHARACTER_POSITION:
			Position(ch, c_pData);
			break;

		case HEADER_CG_ITEM_USE:
			if (!ch->IsObserverMode())
				ItemUse(ch, c_pData);
			break;

		case HEADER_CG_ITEM_DROP:
			if (!ch->IsObserverMode())
			{
				ItemDrop(ch, c_pData);
			}
			break;

		case HEADER_CG_ITEM_DROP2:
			if (!ch->IsObserverMode())
				ItemDrop2(ch, c_pData);
			break;

		case HEADER_CG_ITEM_MOVE:
			if (!ch->IsObserverMode())
				ItemMove(ch, c_pData);
			break;

#ifdef ENABLE_SELL_ITEM
		case HEADER_CG_ITEM_SELL:
			if (!ch->IsObserverMode())
				ItemSell(ch, c_pData);
			break;
#endif

#if defined(__DUNGEON_INFO_SYSTEM__)
		case DungeonInfo::Packet::HEADER_CG_DUNGEON_INFO:
			DungeonInfo(ch, c_pData);
			break;
#endif
			
#ifdef ENABLE_SORT_INVEN
		case SORT_INVEN:
			if (!ch->IsObserverMode())
				SortInven(ch, c_pData);
			break;
#endif
			
#ifdef NEW_ADD_INVENTORY
		case ENVANTER_BLACK:
			if (!ch->IsObserverMode())
				Envanter_pakettiii(ch, c_pData);
			break;
#endif

		case HEADER_CG_ITEM_PICKUP:
			if (!ch->IsObserverMode())
				ItemPickup(ch, c_pData);
			break;

#if defined(__BL_SOUL_ROULETTE__)
		case HEADER_CG_SOUL_ROULETTE:
			if (!ch->IsObserverMode())
				SoulRoulette(ch, c_pData);
			break;
#endif

		case HEADER_CG_ITEM_USE_TO_ITEM:
			if (!ch->IsObserverMode())
				ItemToItem(ch, c_pData);
			break;

		case HEADER_CG_ITEM_GIVE:
			if (!ch->IsObserverMode())
				ItemGive(ch, c_pData);
			break;

		case HEADER_CG_EXCHANGE:
			if (!ch->IsObserverMode())
				Exchange(ch, c_pData);
			break;

		case HEADER_CG_ATTACK:
		case HEADER_CG_SHOOT:
			if (!ch->IsObserverMode())
			{
				Attack(ch, bHeader, c_pData);
			}
			break;

		case HEADER_CG_USE_SKILL:
			if (!ch->IsObserverMode())
				UseSkill(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_ADD:
			QuickslotAdd(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_DEL:
			QuickslotDelete(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_SWAP:
			QuickslotSwap(ch, c_pData);
			break;

		case HEADER_CG_SHOP:
			if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

#ifdef OFFLINE_SHOP
		case HEADER_CG_OFFLINE_SHOP:
			if ((iExtraLen = OfflineShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif

		case HEADER_CG_MESSENGER:
			if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft))<0)
				return -1;
			break;

		case HEADER_CG_ON_CLICK:
			OnClick(ch, c_pData);
			break;

		case HEADER_CG_SYNC_POSITION:
			if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_ADD_FLY_TARGETING:
		case HEADER_CG_FLY_TARGETING:
			FlyTarget(ch, c_pData, bHeader);
			break;

		case HEADER_CG_SCRIPT_BUTTON:
			ScriptButton(ch, c_pData);
			break;

			// SCRIPT_SELECT_ITEM
		case HEADER_CG_SCRIPT_SELECT_ITEM:
			ScriptSelectItem(ch, c_pData);
			break;
			// END_OF_SCRIPT_SELECT_ITEM

		case HEADER_CG_SCRIPT_ANSWER:
			ScriptAnswer(ch, c_pData);
			break;

		case HEADER_CG_QUEST_INPUT_STRING:
			QuestInputString(ch, c_pData);
			break;

		case HEADER_CG_QUEST_CONFIRM:
			QuestConfirm(ch, c_pData);
			break;

		case HEADER_CG_TARGET:
			Target(ch, c_pData);
			break;

		case HEADER_CG_WARP:
			Warp(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKIN:
			SafeboxCheckin(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKOUT:
			SafeboxCheckout(ch, c_pData, false);
			break;

		case HEADER_CG_SAFEBOX_ITEM_MOVE:
			SafeboxItemMove(ch, c_pData);
			break;

#if defined(BL_REMOTE_SHOP)
		case HEADER_CG_REMOTE_SHOP:
			RemoteShop(ch, c_pData);
			break;
#endif

		case HEADER_CG_MALL_CHECKOUT:
			SafeboxCheckout(ch, c_pData, true);
			break;

#if defined(__BL_MAILBOX__)
		case HEADER_CG_MAILBOX_WRITE:
			MailboxWrite(ch, c_pData);
			break;
		
		case HEADER_CG_MAILBOX_WRITE_CONFIRM:
			MailboxConfirm(ch, c_pData);
			break;

		case HEADER_CG_MAILBOX_PROCESS:
			MailboxProcess(ch, c_pData);
			break;
#endif

		case HEADER_CG_PARTY_INVITE:
			PartyInvite(ch, c_pData);
			break;

		case HEADER_CG_PARTY_REMOVE:
			PartyRemove(ch, c_pData);
			break;

		case HEADER_CG_PARTY_INVITE_ANSWER:
			PartyInviteAnswer(ch, c_pData);
			break;

		case HEADER_CG_PARTY_SET_STATE:
			PartySetState(ch, c_pData);
			break;

		case HEADER_CG_PARTY_USE_SKILL:
			PartyUseSkill(ch, c_pData);
			break;

		case HEADER_CG_PARTY_PARAMETER:
			PartyParameter(ch, c_pData);
			break;

#ifdef __INGAME_WIKI__
		case InGameWiki::HEADER_CG_WIKI:
			RecvWikiPacket(ch, c_pData);
			break;
#endif

		case HEADER_CG_ANSWER_MAKE_GUILD:
#ifdef ENABLE_NEWGUILDMAKE
			ch->ChatPacket(CHAT_TYPE_INFO, "<%s> AnswerMakeGuild disabled", __FUNCTION__);
#else
			AnswerMakeGuild(ch, c_pData);
#endif
			break;

		case HEADER_CG_GUILD:
			if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_FISHING:
			Fishing(ch, c_pData);
			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;
#ifdef ENABLE_GUILD_REQUEST
	case HEADER_CG_GUILD_REQUEST:
		if ((iExtraLen = GuildRequest(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif			
#ifdef NEW_PET_SYSTEM
		case HEADER_CG_PetSetName:
			BraveRequestPetName(ch, c_pData);
			break;
#endif

		case HEADER_CG_MYSHOP:
			if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

#ifdef OFFLINE_SHOP
		case HEADER_CG_MY_OFFLINE_SHOP:
			if ((iExtraLen = MyOfflineShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif

		case HEADER_CG_REFINE:
			Refine(ch, c_pData);
			break;
			
#ifdef ENABLE_CSHIELD
		case HEADER_CG_CSHIELD:
			CShieldReceive(ch, c_pData);
			break;
#endif

#ifdef PRIVATESHOP_SEARCH_SYSTEM
		case HEADER_CG_PSHOP_SEARCH:
			ShopSearchInfo(ch, c_pData, false);
			break;

		case HEADER_CG_PSHOP_SEARCH_NAME_ONLY:
			ShopSearchInfo(ch, c_pData, true);
			break;

		case HEADER_CG_PSHOP_SEARCH_BUY:
			ShopSearchBuyItem(ch, c_pData);
			break;
#endif

#ifdef CHANGELOOK_SYSTEM
		case HEADER_CG_CL:
		{
			ChangeLook(ch, c_pData);
		}
		break;
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		case HEADER_CG_REQUEST_FLAG:
			RequestFlag(ch, c_pData);
			break;	
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case HEADER_CG_ACCE:
		{
			Acce(ch, c_pData);
		}
		break;
#endif
		case HEADER_CG_CLIENT_VERSION:
			Version(ch, c_pData);
			break;

		case HEADER_CG_HS_ACK:
			if (isHackShieldEnable)
			{
				CHackShieldManager::instance().VerifyAck(d->GetCharacter(), c_pData);
			}
			break;

		case HEADER_CG_XTRAP_ACK:
			{
				TPacketXTrapCSVerify* p = reinterpret_cast<TPacketXTrapCSVerify*>((void*)c_pData);
				CXTrapManager::instance().Verify_CSStep3(d->GetCharacter(), p->bPacketData);
			}
			break;
#ifdef TARGET_INFORMATION_SYSTEM
		case HEADER_CG_TARGET_INFO_LOAD:
		{
			TargetInfoLoad(ch, c_pData);
		}
		break;
#endif

#ifdef ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
		case HEADER_CG_BUY_BONUS_CHANGER_IN_SWITCH_BOT:
			BuyBonusChangerInSwitchBot(ch);
			break;		
#endif

#ifdef ENABLE_SWITCHBOT
		case HEADER_CG_SWITCHBOT:
			if ((iExtraLen = Switchbot(ch, c_pData, m_iBufferLeft)) < 0)
			{
				return -1;
			}
			break;
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
		case HEADER_CG_CUBE_RENEWAL:
			CubeRenewalSend(ch, c_pData);
			break;
#endif
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
		case HEADER_CG_67_BONUS_NEW:
			Bonus67NewSend(ch, c_pData);
			break;

#endif
#ifdef ENABLE_HUNTING_SYSTEM
		case HEADER_CG_SEND_HUNTING_ACTION:
			ReciveHuntingAction(ch, c_pData);
			break;
#endif
#ifdef __ULTIMATE_TOOLTIP__
		case HEADER_CG_CHEST_DROP_INFO:
			ChestDropInfo(ch, c_pData);
			break;
#endif
#ifdef WON_EXCHANGE
		case HEADER_CG_WON_EXCHANGE:
			WonExchange(ch, c_pData);
			break;
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
		case HEADER_CG_ANTI_FARM:
			if ((iExtraLen = RecvAntiFarmUpdateStatus(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif
#ifdef ENABLE_NEW_FISHING_SYSTEM
		case HEADER_CG_FISHING_NEW:
			{
				FishingNew(ch, c_pData);
			}
			break;
#endif
		case HEADER_CG_DRAGON_SOUL_REFINE:
			{
				TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
				switch(p->bSubType)
				{
				case DS_SUB_HEADER_CLOSE:
					ch->DragonSoul_RefineWindow_Close();
					break;
				case DS_SUB_HEADER_DO_REFINE_GRADE:
					{
						DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STEP:
					{
						DSManager::instance().DoRefineStep(ch, p->ItemGrid);
					}
					break;

				case DS_SUB_HEADER_DO_REFINE_STRENGTH:
					{
						DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
					}
					break;
				}
			}
			break;
	}
	return (iExtraLen);

}

#if defined(__DUNGEON_INFO_SYSTEM__)
void CInputMain::DungeonInfo(LPCHARACTER ch, const char* c_pData)
{
	DungeonInfo::Packet::CGInfo* CGPacket = (DungeonInfo::Packet::CGInfo*)c_pData;

	switch (CGPacket->bySubHeader)
	{
	case DungeonInfo::EAction::CLOSE:
	{
		ch->SetDungeonInfoOpen(false);
		ch->StopDungeonInfoReloadEvent();
	}
	break;

	case DungeonInfo::EAction::OPEN:
		if (!ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().SendInfo(ch);
		break;

	case DungeonInfo::EAction::WARP:
		if (ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().Warp(ch, CGPacket->byIndex);
		break;

	case DungeonInfo::EAction::RANK:
		if (ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().Ranking(ch, CGPacket->byIndex, CGPacket->byRankType);
		break;

	default:
		return;
	}
}
#endif

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		return 0;
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;
#ifdef ANTY_WAIT_HACK
		case HEADER_CG_ANTY_WAIT_HACK:
		{
			LPCHARACTER ch;

			if (!(ch = d->GetCharacter()))
			{
				sys_err("no character on desc");
				return 0;
			}
			TPacketCGAntyWH* p = reinterpret_cast <TPacketCGAntyWH*>((void*)c_pData);

			if (AntyWHSums.find(ch->GetRealJob()) != AntyWHSums.end())
			{

				sys_log(1, "Checking file %s hash %s", p->file, AntyWHSums[ch->GetRealJob()].c_str());
				ch->SetLastWHRegex(p->file);
				if (AntyWHSums[ch->GetRealJob()] == p->hash)
					ch->SetWHPass();
			}
	}
#endif
		case HEADER_CG_CHAT:
			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;

		default:
			return (0);
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
int CInputMain::Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGSwitchbot* p = reinterpret_cast<const TPacketCGSwitchbot*>(data);

	if (uiBytes < sizeof(TPacketCGSwitchbot))
	{
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGSwitchbot);
	uiBytes -= sizeof(TPacketCGSwitchbot);

	switch (p->subheader)
	{
	case SUBHEADER_CG_SWITCHBOT_START:
	{
		size_t extraLen = sizeof(TSwitchbotAttributeAlternativeTable) * SWITCHBOT_ALTERNATIVE_COUNT;
		if (uiBytes < extraLen)
		{
			return -1;
		}

		std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives;

		for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
		{
			const TSwitchbotAttributeAlternativeTable* pAttr = reinterpret_cast<const TSwitchbotAttributeAlternativeTable*>(c_pData);
			c_pData += sizeof(TSwitchbotAttributeAlternativeTable);

			vec_alternatives.push_back(*pAttr);
		}

		CSwitchbotManager::Instance().Start(ch->GetPlayerID(), p->slot, vec_alternatives);
		return extraLen;
	}

	case SUBHEADER_CG_SWITCHBOT_STOP:
	{
		CSwitchbotManager::Instance().Stop(ch->GetPlayerID(), p->slot);
		return 0;
	}
	}

	return 0;
}
#endif

#ifdef __ULTIMATE_TOOLTIP__
void CInputMain::ChestDropInfo(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGChestDropInfo* p = (TPacketCGChestDropInfo*) c_pData;
	TPacketGCChestDropInfo packet;
	std::vector<TChestDropInfoTable> vec_ItemList;
	if(p->item_vnum>0)
	{
		ITEM_MANAGER::instance().GetChestItemList(p->item_vnum, vec_ItemList);
		packet.bHeader = HEADER_GC_CHEST_DROP_INFO;
		packet.wSize = sizeof(packet) + sizeof(TChestDropInfoTable) * vec_ItemList.size();
		packet.dwChestVnum = p->item_vnum;
		packet.sub = 1;
		ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
		ch->GetDesc()->Packet(&vec_ItemList[0], sizeof(TChestDropInfoTable) * vec_ItemList.size());
	}
	else
	{
		if(p->wInventoryCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			return;
		LPITEM pkItem = ch->GetInventoryItem(p->wInventoryCell);
		if (!pkItem)
			return;
		ITEM_MANAGER::instance().GetChestItemList(pkItem->GetVnum(), vec_ItemList);
		packet.bHeader = HEADER_GC_CHEST_DROP_INFO;
		packet.sub = 0;
		packet.wSize = sizeof(packet) + sizeof(TChestDropInfoTable) * vec_ItemList.size();
		packet.dwChestVnum = pkItem->GetVnum();
		ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
		ch->GetDesc()->Packet(&vec_ItemList[0], sizeof(TChestDropInfoTable) * vec_ItemList.size());
	}
}
#endif

#ifdef WON_EXCHANGE
void CInputMain::WonExchange(LPCHARACTER ch, const char* pcData)
{
	const TPacketCGWonExchange* p = reinterpret_cast<const TPacketCGWonExchange*>(pcData);
	const EWonExchangeCGSubHeader SubHeader = static_cast<EWonExchangeCGSubHeader>(p->bSubHeader);
	switch (SubHeader)
	{
	case WON_EXCHANGE_CG_SUBHEADER_SELL:
	case WON_EXCHANGE_CG_SUBHEADER_BUY:
		ch->WonExchange(SubHeader, p->wValue);
		break;
	default:
		sys_err("invalid won exchange subheader: %u value: %u", SubHeader, p->wValue);
		break;
	}
}

#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
auto CInputMain::RecvAntiFarmUpdateStatus(LPCHARACTER ch, const char* data, size_t uiBytes) -> int
{
	const TSendAntiFarmInfo* p = reinterpret_cast<const TSendAntiFarmInfo*>(data);

	if (uiBytes < sizeof(TSendAntiFarmInfo))
		return -1;

	LPDESC d = nullptr;
	if (!ch || !(d = ch->GetDesc()))
		return -1;

	const char* c_pData = data + sizeof(TSendAntiFarmInfo);
	uiBytes -= sizeof(TSendAntiFarmInfo);

	switch (p->subheader)
	{
	case AF_SH_SEND_STATUS_UPDATE:
	{
		size_t extraLen = (sizeof(DWORD) * MULTIPLE_FARM_MAX_ACCOUNT);
		if (uiBytes < extraLen)
			return -1;

		std::vector<DWORD> v_dwPIDS;
		for (uint8_t i = 0; i < MULTIPLE_FARM_MAX_ACCOUNT; ++i)
			v_dwPIDS.emplace_back(*reinterpret_cast<const DWORD*>(c_pData + (sizeof(DWORD) * i)));

		std::string sMAIf = d->GetLoginMacAdress();
		CAntiMultipleFarm::instance().SendBlockDropStatusChange(sMAIf, v_dwPIDS);

		{ //send p2p update
			CAntiMultipleFarm::TP2PChangeDropStatus dataPacket(HEADER_GG_ANTI_FARM);
			strlcpy(dataPacket.cMAIf, sMAIf.c_str(), sizeof(dataPacket.cMAIf));
			for (uint8_t i = 0; i < v_dwPIDS.size(); ++i)
				dataPacket.dwPIDs[i] = v_dwPIDS[i];
			P2P_MANAGER::instance().Send(&dataPacket, sizeof(CAntiMultipleFarm::TP2PChangeDropStatus));
		}

		return extraLen;
	}
	}

	return 0;
}
#endif

#ifdef ENABLE_GUILD_REQUEST
int CInputMain::GuildRequest(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGGuildRequest* p = (TPacketCGGuildRequest*)data;
	if (p->sub_index == SUB_REQUEST_PAGEINDEX)
	{
		int extraLen = sizeof(BYTE) + sizeof(int);
		if (uiBytes < (sizeof(TPacketCGGuildRequest) + extraLen))
			return -1;
		data += sizeof(TPacketCGGuildRequest);
		const BYTE tabIndex = *(BYTE*)data;
		data += sizeof(BYTE);
		const int pageIndex = *(int*)data;
		CGuildManager::Instance().SendData(ch, tabIndex, pageIndex, NULL);
		return extraLen;
	}
	else if (p->sub_index == SUB_REQUEST_NAME)
	{
		struct req_name{char name[GUILD_NAME_MAX_LEN + 1];};
		int extraLen = (sizeof(BYTE)+sizeof(req_name));
		if (uiBytes < (sizeof(TPacketCGGuildRequest) + extraLen))
			return -1;
		data += sizeof(TPacketCGGuildRequest);
		const BYTE tabIndex = *(BYTE*)data;
		data += sizeof(BYTE);
		const req_name& request_name = *(req_name*)data;
		CGuildManager::Instance().SendData(ch, tabIndex, 0, request_name.name);
		return extraLen;
	}
	else if (p->sub_index == SUB_REQUEST_REQUEST)
	{
		int extraLen = (sizeof(DWORD)+sizeof(BYTE));
		if (uiBytes < (sizeof(TPacketCGGuildRequest) + extraLen))
			return -1;
		data += sizeof(TPacketCGGuildRequest);
		const DWORD guildID = *(DWORD*)data;
		data += sizeof(DWORD);
		const BYTE requestIndex = *(BYTE*)data;
		CGuildManager::Instance().SendRequest(ch, guildID, requestIndex);
		return extraLen;
	}
	else
	{
		sys_err("guild_request unknown sub_index %d", p->sub_index);
	}
	return -1;
}
#endif

#ifdef ENABLE_NEW_FISHING_SYSTEM
void CInputMain::FishingNew(LPCHARACTER ch, const char* c_pData)
{
	if (!ch) {
		return;
	}

	TPacketFishingNew* p = (TPacketFishingNew*)c_pData;
	switch (p->subheader) {
		case FISHING_SUBHEADER_NEW_START:
			{
				ch->SetRotation(p->dir * 5);
				ch->fishing_new_start();
			}
			break;
		case FISHING_SUBHEADER_NEW_STOP:
			{
				ch->SetRotation(p->dir * 5);
				ch->fishing_new_stop();
			}
			break;
		case FISHING_SUBHEADER_NEW_CATCH:
			{
				ch->fishing_new_catch();
			}
			break;
		case FISHING_SUBHEADER_NEW_CATCH_FAILED:
			{
				ch->fishing_new_catch_failed();
			}
			break;
		default:
			return;
	}
}
#endif