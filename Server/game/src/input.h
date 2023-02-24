#ifndef __INC_METIN_II_GAME_INPUT_PROCESSOR__
#define __INC_METIN_II_GAME_INPUT_PROCESSOR__

#include "packet_info.h"
#include "../../common/CommonDefines.h"
enum
{
	INPROC_CLOSE,
	INPROC_HANDSHAKE,
	INPROC_LOGIN,
	INPROC_MAIN,
	INPROC_DEAD,
	INPROC_DB,
	INPROC_UDP,
	INPROC_P2P,
	INPROC_AUTH,
	INPROC_TEEN,
};

void LoginFailure(LPDESC d, const char * c_pszStatus);
extern void SendShout(const char * szText, BYTE bEmpire);

class CInputProcessor
{
	public:
		CInputProcessor();
		virtual ~CInputProcessor() {};

		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
		virtual BYTE GetType() = 0;

		void BindPacketInfo(CPacketInfo * pPacketInfo);
		void Pong(LPDESC d);
		void Handshake(LPDESC d, const char * c_pData);
		void Version(LPCHARACTER ch, const char* c_pData);

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData) = 0;

		CPacketInfo * m_pPacketInfo;
		int	m_iBufferLeft;

		CPacketInfoCG 	m_packetInfoCG;
};

class CInputClose : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_CLOSE; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData) { return m_iBufferLeft; }
};

class CInputHandshake : public CInputProcessor
{
	public:
		CInputHandshake();
		virtual ~CInputHandshake();

		virtual BYTE	GetType() { return INPROC_HANDSHAKE; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		GuildMarkLogin(LPDESC d, const char* c_pData);

		CPacketInfo *	m_pMainPacketInfo;
};

class CInputLogin : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_LOGIN; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Login(LPDESC d, const char * data);
		void		LoginByKey(LPDESC d, const char * data);

		void		CharacterSelect(LPDESC d, const char * data);
		void		CharacterCreate(LPDESC d, const char * data);
		void		CharacterDelete(LPDESC d, const char * data);
		void		Entergame(LPDESC d, const char * data);
		void		Empire(LPDESC d, const char * c_pData);
		void		GuildMarkCRCList(LPDESC d, const char* c_pData);
		// MARK_BUG_FIX
		void		GuildMarkIDXList(LPDESC d, const char* c_pData);
		// END_OF_MARK_BUG_FIX
		void		GuildMarkUpload(LPDESC d, const char* c_pData);
		int			GuildSymbolUpload(LPDESC d, const char* c_pData, size_t uiBytes);
		void		GuildSymbolCRC(LPDESC d, const char* c_pData);
		void		ChangeName(LPDESC d, const char * data);
};

class CInputMain : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_MAIN; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Attack(LPCHARACTER ch, const BYTE header, const char* data);

		int			Whisper(LPCHARACTER ch, const char * data, size_t uiBytes);
#ifdef NEW_ADD_INVENTORY
		void		Envanter_pakettiii(LPCHARACTER ch, const char * data);
#endif
#ifdef NEW_PET_SYSTEM
		void		BraveRequestPetName(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef WON_EXCHANGE
		void		WonExchange(LPCHARACTER ch, const char* pcData);
#endif
		int			Chat(LPCHARACTER ch, const char * data, size_t uiBytes);
#ifdef ENABLE_GUILD_REQUEST
		int			GuildRequest(LPCHARACTER ch, const char* data, size_t uiBytes);
#endif
		void		ItemUse(LPCHARACTER ch, const char * data);
		void		ItemDrop(LPCHARACTER ch, const char * data);
		void		ItemDrop2(LPCHARACTER ch, const char * data);
		void		ItemMove(LPCHARACTER ch, const char * data);
#ifdef ENABLE_SORT_INVEN
		void		SortInven(LPCHARACTER ch, const char * data);
#endif
		void		ItemPickup(LPCHARACTER ch, const char * data);
#if defined(__BL_SOUL_ROULETTE__)
		void		SoulRoulette(LPCHARACTER ch, const char* data);
#endif		
		void		ItemToItem(LPCHARACTER ch, const char * pcData);
		void		QuickslotAdd(LPCHARACTER ch, const char * data);
		void		QuickslotDelete(LPCHARACTER ch, const char * data);
		void		QuickslotSwap(LPCHARACTER ch, const char * data);
		int			Shop(LPCHARACTER ch, const char * data, size_t uiBytes);

		void		OnClick(LPCHARACTER ch, const char * data);
		void		Exchange(LPCHARACTER ch, const char * data);
		void		Position(LPCHARACTER ch, const char * data);
		void		Move(LPCHARACTER ch, const char * data);
		int			SyncPosition(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		FlyTarget(LPCHARACTER ch, const char * pcData, BYTE bHeader);
		void		UseSkill(LPCHARACTER ch, const char * pcData);

		void		ScriptAnswer(LPCHARACTER ch, const void * pvData);
		void		ScriptButton(LPCHARACTER ch, const void * pvData);
		void		ScriptSelectItem(LPCHARACTER ch, const void * pvData);

		void		QuestInputString(LPCHARACTER ch, const void * pvData);
		void		QuestConfirm(LPCHARACTER ch, const void* pvData);
		void		Target(LPCHARACTER ch, const char * pcData);
		void		Warp(LPCHARACTER ch, const char * pcData);
		void		SafeboxCheckin(LPCHARACTER ch, const char * c_pData);
		#ifdef ENABLE_BUY_BONUS_CHANGER_IN_SWITCH_BOT
		void		BuyBonusChangerInSwitchBot( LPCHARACTER ch );
		#endif
		
		void		SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall);
		void		SafeboxItemMove(LPCHARACTER ch, const char * data);
#ifdef ENABLE_HUNTING_SYSTEM
		int			ReciveHuntingAction(LPCHARACTER ch, const char* c_pData);
#endif
		int			Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes);
#if defined(__BL_MAILBOX__)
		void		MailboxWrite(LPCHARACTER ch, const char* data);
		void		MailboxConfirm(LPCHARACTER ch, const char* data);
		void		MailboxProcess(LPCHARACTER ch, const char* c_pData);
#endif	
#if defined(BL_REMOTE_SHOP)
		void		RemoteShop(LPCHARACTER ch, const char* data);
#endif
		void 		PartyInvite(LPCHARACTER ch, const char * c_pData);
		void 		PartyInviteAnswer(LPCHARACTER ch, const char * c_pData);
		void		PartyRemove(LPCHARACTER ch, const char * c_pData);
		void		PartySetState(LPCHARACTER ch, const char * c_pData);
		void		PartyUseSkill(LPCHARACTER ch, const char * c_pData);
		void		PartyParameter(LPCHARACTER ch, const char * c_pData);
#ifdef __INGAME_WIKI__
		void		RecvWikiPacket(LPCHARACTER ch, const char * c_pData);
#endif

		int			Guild(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		AnswerMakeGuild(LPCHARACTER ch, const char* c_pData);

		void		Fishing(LPCHARACTER ch, const char* c_pData);
		void		ItemGive(LPCHARACTER ch, const char* c_pData);
		void		Hack(LPCHARACTER ch, const char * c_pData);
#ifdef __ULTIMATE_TOOLTIP__
		void		ChestDropInfo(LPCHARACTER ch, const char * c_pData);
#endif
		int			MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes);
#ifdef ENABLE_ANTI_MULTIPLE_FARM
		auto		RecvAntiFarmUpdateStatus(LPCHARACTER ch, const char* c_pData, size_t uiBytes) -> int;
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
		void 		CubeRenewalSend(LPCHARACTER ch, const char* data);
#endif
		void		Refine(LPCHARACTER ch, const char* c_pData);
#ifdef ENABLE_SELL_ITEM
		void		ItemSell(LPCHARACTER ch, const char * data);
#endif
#ifdef ENABLE_CSHIELD
		void		CShieldReceive(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef SYSTEM_PDA
		void		SoulStoneUse(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef ENABLE_6_7_BONUS_NEW_SYSTEM
		void 		Bonus67NewSend(LPCHARACTER ch, const char * data);
#endif
#ifdef TARGET_INFORMATION_SYSTEM
		void		TargetInfoLoad(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void		Acce(LPCHARACTER pkChar, const char* c_pData);
#endif
#ifdef CHANGELOOK_SYSTEM
		void	ChangeLook(LPCHARACTER pkChar, const char* c_pData);
#endif
		void		Roulette(LPCHARACTER ch, const char* c_pData);
#if defined(__DUNGEON_INFO_SYSTEM__)
		void DungeonInfo(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef OFFLINE_SHOP
		int			OfflineShop(LPCHARACTER ch, const char * data, size_t uiBytes);
		int			MyOfflineShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes);
#endif
#ifdef PRIVATESHOP_SEARCH_SYSTEM
		void 		ShopSearchInfo(LPCHARACTER ch, const char* data, bool bNameOnly);
		void		ShopSearchBuyItem(LPCHARACTER ch, const char* data);
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		std::string GetLanguageFlag(const char* c_pszName);
		std::string GetEmpireFlag(const char* c_pszName);
		std::string ConvertEmpireIndexToStr(int empireIndex);
		void		RequestFlag(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef ENABLE_NEW_FISHING_SYSTEM
		void FishingNew(LPCHARACTER ch, const char* c_pData);
#endif
#ifdef ENABLE_SWITCHBOT
		int			Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes);
#endif		
};

class CInputDead : public CInputMain
{
	public:
		virtual BYTE	GetType() { return INPROC_DEAD; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);
};

class CInputDB : public CInputProcessor
{
public:
	virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
	virtual BYTE GetType() { return INPROC_DB; }

protected:
	virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

protected:
	void		MapLocations(const char * c_pData);
	void		LoginSuccess(DWORD dwHandle, const char *data);
	void		PlayerCreateFailure(LPDESC d, BYTE bType);	// 0 = �Ϲ� ���� 1 = �̹� ����
	void		PlayerDeleteSuccess(LPDESC d, const char * data);
	void		PlayerDeleteFail(LPDESC d);
	void		PlayerLoad(LPDESC d, const char* data);
	void		PlayerCreateSuccess(LPDESC d, const char * data);
	void		Boot(const char* data);
	void		QuestLoad(LPDESC d, const char * c_pData);
#if defined(__BL_MAILBOX__)
	void		MailBoxRespondLoad(LPDESC d, const char * c_pData);
	void		MailBoxRespondName(LPDESC d, const char * c_pData);
	void		MailBoxRespondUnreadData(LPDESC d, const char * c_pData);
#endif
#ifdef ENABLE_DECORUM
	void		DecorumLoad(LPDESC d, const char * c_pData);
	void		BroadcastedDecurumSeasonResult(const char * c_pData);
#endif	
	void		SafeboxLoad(LPDESC d, const char * c_pData);
	void		SafeboxChangeSize(LPDESC d, const char * c_pData);
	void		SafeboxWrongPassword(LPDESC d);
	void		SafeboxChangePasswordAnswer(LPDESC d, const char* c_pData);
	void		MallLoad(LPDESC d, const char * c_pData);
	void		EmpireSelect(LPDESC d, const char * c_pData);
	void		P2P(const char * c_pData);
	void		ItemLoad(LPDESC d, const char * c_pData);
	void		AffectLoad(LPDESC d, const char * c_pData);
	void		MarriageLoad(LPDESC d, const char * c_pData);
	void		GuildLoad(const char * c_pData);
#ifdef GUILD_WAR_COUNTER
	void		GuildCounter(LPDESC d, const char* c_pData);
#endif
	void		GuildSkillUpdate(const char* c_pData);
	void		GuildSkillRecharge();
	void		GuildExpUpdate(const char* c_pData);
	void		GuildAddMember(const char* c_pData);
	void		GuildRemoveMember(const char* c_pData);
	void		GuildChangeGrade(const char* c_pData);
	void		GuildChangeMemberData(const char* c_pData);
	void		GuildDisband(const char* c_pData);
	void		GuildLadder(const char* c_pData);
	void		GuildWar(const char* c_pData);
	void		GuildWarScore(const char* c_pData);
	void		GuildSkillUsableChange(const char* c_pData);
	void		GuildMoneyChange(const char* c_pData);
	void		GuildWithdrawMoney(const char* c_pData);
	void		GuildWarReserveAdd(TGuildWarReserve * p);
	void		GuildWarReserveUpdate(TGuildWarReserve * p);
	void		GuildWarReserveDelete(DWORD dwID);
	void		GuildWarBet(TPacketGDGuildWarBet * p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		LoginAlready(LPDESC d, const char * c_pData);

	void		PartyCreate(const char* c_pData);
	void		PartyDelete(const char* c_pData);
	void		PartyAdd(const char* c_pData);
	void		PartyRemove(const char* c_pData);
	void		PartyStateChange(const char* c_pData);
	void		PartySetMemberLevel(const char* c_pData);

	void		Time(const char * c_pData);

	void		ReloadProto(const char * c_pData);
	void		ChangeName(LPDESC d, const char * data);

	void		AuthLogin(LPDESC d, const char * c_pData);
	void		AuthLoginOpenID(LPDESC d, const char * c_pData);
	void		ItemAward(const char * c_pData);

	void		ChangeEmpirePriv(const char* c_pData);
	void		ChangeGuildPriv(const char* c_pData);
	void		ChangeCharacterPriv(const char* c_pData);

	void		MoneyLog(const char* c_pData);

	void		SetEventFlag(const char* c_pData);

	void		BillingRepair(const char * c_pData);
	void		BillingExpire(const char * c_pData);
	void		BillingLogin(const char * c_pData);
	void		BillingCheck(const char * c_pData);
	void		VCard(const char * c_pData);

	void		CreateObject(const char * c_pData);
	void		DeleteObject(const char * c_pData);
	void		UpdateLand(const char * c_pData);

#ifdef ENABLE_SPECIAL_AFFECT
	void		SpecialAffect(TSpecialAffects* p);
#endif

	void		Notice(const char * c_pData);

	void		MarriageAdd(TPacketMarriageAdd * p);
	void		MarriageUpdate(TPacketMarriageUpdate * p);
	void		MarriageRemove(TPacketMarriageRemove * p);
#ifdef OFFLINE_SHOP
	void		HandleCurrentOnline(TPacketOnlineSize * p);
#endif
	void		WeddingRequest(TPacketWeddingRequest* p);
	void		WeddingReady(TPacketWeddingReady* p);
	void		WeddingStart(TPacketWeddingStart* p);
	void		WeddingEnd(TPacketWeddingEnd* p);

	void		TakeMonarchMoney(LPDESC d, const char * data );
	void		AddMonarchMoney(LPDESC d, const char * data );
	void		DecMonarchMoney(LPDESC d, const char * data );
	void		SetMonarch( LPDESC d, const char * data );

	void		ChangeMonarchLord(TPacketChangeMonarchLordACK* data);
	void		UpdateMonarchInfo(TMonarchInfo* data);
	void		AddBlockCountryIp(TPacketBlockCountryIp * data);
	void		BlockException(TPacketBlockException * data);

	// MYSHOP_PRICE_LIST
	/// ������ �������� ����Ʈ ��û�� ���� ���� ��Ŷ(HEADER_DG_MYSHOP_PRICELIST_RES) ó���Լ�
	/**
	* @param	d ������ �������� ����Ʈ�� ��û�� �÷��̾��� descriptor
	* @param	p ��Ŷ�������� ������
	*/
	void		MyshopPricelistRes( LPDESC d, const TPacketMyshopPricelistHeader* p );
	// END_OF_MYSHOP_PRICE_LIST
	//
	//RELOAD_ADMIN
	void ReloadAdmin( const char * c_pData );
	//END_RELOAD_ADMIN

#ifdef ENABLE_EVENT_MANAGER
	void		EventManager(const char* c_pData);
#endif

	void		DetailLog(const TPacketNeedLoginLogInfo* info);
	// ���� ���� ��� �׽�Ʈ
	void		ItemAwardInformer(TPacketItemAwardInfromer* data);

	void		RespondChannelStatus(LPDESC desc, const char* pcData);

#ifdef __SPECIALSTAT_SYSTEM__
	void		LoadSpecialStats(LPDESC desc, const char * data);
#endif

	protected:
		DWORD		m_dwHandle;
};

class CInputUDP : public CInputProcessor
{
	public:
		CInputUDP();
		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);

		virtual BYTE GetType() { return INPROC_UDP; }
		void		SetSockAddr(struct sockaddr_in & rSockAddr) { m_SockAddr = rSockAddr; };

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	protected:
		void		Handshake(LPDESC lpDesc, const char * c_pData);
		void		StateChecker(const char * c_pData);

	protected:
		struct sockaddr_in	m_SockAddr;
		CPacketInfoUDP 		m_packetInfoUDP;
};

class CInputP2P : public CInputProcessor
{
	public:
		CInputP2P();
		virtual BYTE	GetType() { return INPROC_P2P; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);

	public:
		void		Setup(LPDESC d, const char * c_pData);
		void		Login(LPDESC d, const char * c_pData);
		void		Logout(LPDESC d, const char * c_pData);
		int			Relay(LPDESC d, const char * c_pData, size_t uiBytes);
#ifdef __WORLD_BOSS_YUMA__
		int			NewNotice(LPDESC d, const char* c_pData, size_t uiBytes);
#endif
#ifdef ENABLE_FULL_NOTICE
		int			Notice(LPDESC d, const char * c_pData, size_t uiBytes, bool bBigFont=false);
#else
		int			Notice(LPDESC d, const char * c_pData, size_t uiBytes);
#endif
		int			MonarchNotice(LPDESC d, const char * c_pData, size_t uiBytes);
		int			MonarchTransfer(LPDESC d, const char * c_pData);
		int			Guild(LPDESC d, const char* c_pData, size_t uiBytes);
#ifdef ENABLE_MAINTENANCE_SYSTEM
		void		RecvShutdown(LPDESC d, const char * c_pData);
#endif
		void		Shout(const char * c_pData);
		void		Disconnect(const char * c_pData);
		void		MessengerAdd(const char * c_pData);
		void		MessengerRemove(const char * c_pData);
#ifdef ENABLE_MESSENGER_BLOCK
		void		MessengerBlockAdd(const char * c_pData);
		void		MessengerBlockRemove(const char * c_pData);
#endif
		void		MessengerMobile(const char * c_pData);
		void		FindPosition(LPDESC d, const char* c_pData);
		void		WarpCharacter(const char* c_pData);
		void		GuildWarZoneMapIndex(const char* c_pData);
		void		Transfer(const char * c_pData);
		void		XmasWarpSanta(const char * c_pData);
		void		XmasWarpSantaReply(const char * c_pData);
		void		LoginPing(LPDESC d, const char * c_pData);
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
		void		MessengerRequestAdd(const char* c_pData);
#endif
		void		BlockChat(const char * c_pData);
		void		PCBangUpdate(const char* c_pData);
		void		IamAwake(LPDESC d, const char * c_pData);
#ifdef ENABLE_ANTI_MULTIPLE_FARM
		auto		RecvAntiFarmUpdateStatus(LPDESC d, const char* c_pData) -> void;
#endif
#ifdef ENABLE_MAINTENANCE_SYSTEM
		int			PlayerPacket(const char * c_pData, size_t uiBytes);
#endif
#ifdef ENABLE_DECORUM
		int			DecorumArenaStart(LPDESC d, const char * c_pData, size_t uiBytes);
		int			DecorumRandomRequest(LPDESC d, const char * c_pData, size_t uiBytes);
		void		DecorumRandomDelete(LPDESC d, const char * c_pData);
		void		DecorumRandomBroadcast(LPDESC d, const char * c_pData);
		void		DecorumArenaWarpIn(const char * c_pData);
		
#endif		
#ifdef OFFLINE_SHOP
		void		SendOfflineShopMessage(LPDESC d, const char * c_pData);
#endif
#ifdef ENABLE_SWITCHBOT
		void		Switchbot(LPDESC d, const char* c_pData);
#endif
	protected:
		CPacketInfoGG 	m_packetInfoGG;
};

class CInputAuth : public CInputProcessor
{
	public:
		CInputAuth();
		virtual BYTE GetType() { return INPROC_AUTH; }

	protected:
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData);
		int auth_OpenID(const char *authKey, const char *ipAddr, char *rID);

	public:
		void		Login(LPDESC d, const char * c_pData);
		void		LoginOpenID(LPDESC d, const char * c_pData);		//2012.07.19 OpenID : ����
		void		PasspodAnswer(LPDESC d, const char * c_pData );

};

class CInputTeen : public CInputProcessor
{
	public :
		virtual BYTE GetType() { return INPROC_TEEN; }

		void SetStep(int step);

	protected :
		virtual bool Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
		virtual int	Analyze(LPDESC d, BYTE bHeader, const char * c_pData) { return 0; };

	private:
		int	m_step;

		bool ProcessHandshake(LPDESC lpDesc, const void * c_pvOrig, size_t uiBytes, int & r_iBytesProceed);
		bool ProcessMain(LPDESC lpDesc, const void * c_pvOrig, size_t uiBytes, int & r_iBytesProceed);
};

#endif /* __INC_METIN_II_GAME_INPUT_PROCESSOR__ */


