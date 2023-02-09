#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		#ifdef ENABLE_MESSENGER_BLOCK
		typedef std::string keyBL;
		#endif
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);
		bool	AuthToAdd(keyA account, keyA companion, bool bDeny); // @fixme130 void -> bool
#ifdef CROSS_CHANNEL_FRIEND_REQUEST
	void	RegisterRequestToAdd(const char* szAccount, const char* szTarget);
	void	P2PRequestToAdd_Stage1(LPCHARACTER ch, const char* targetName);
	void	P2PRequestToAdd_Stage2(const char* characterName, LPCHARACTER target);
#endif
		void	__AddToList(keyA account, keyA companion);	// 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion); // 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	RemoveFromList(keyA account, keyA companion);
		#ifdef ENABLE_MESSENGER_BLOCK
		void	__AddToBlockList(keyA account, keyA companion);
		void	AddToBlockList(keyA account, keyA companion);
		bool	CheckMessengerList(std::string ch, std::string tch, BYTE type);
		void	__RemoveFromBlockList(keyA account, keyA companion);
		void	RemoveFromBlockList(keyA account, keyA companion);
		void	RemoveAllBlockList(keyA account);
		#endif

		void	RemoveAllList(keyA account);

		void	Initialize();

	private:
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		void	LoadList(SQLMsg * pmsg);
#ifdef ENABLE_MESSENGER_TEAM
		void	SendTeamLogin(keyA account, keyA companion);
		void	SendTeamLogout(keyA account, keyA companion);
		void	LoadTeamList(SQLMsg * pmsg);
		void	SendTeamList(keyA account);
#endif		
		#ifdef ENABLE_MESSENGER_BLOCK
		void	SendBlockLogin(keyA account, keyA companion);
		void	SendBlockLogout(keyA account, keyA companion);
		void	LoadBlockList(SQLMsg * pmsg);
		void	SendBlockList(keyA account);
		#endif
		void	Destroy();

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;
#ifdef ENABLE_MESSENGER_TEAM
		std::map<keyT, std::set<keyT> >	m_TeamRelation;
		std::map<keyT, std::set<keyT> >	m_InverseTeamRelation;
#endif		
		#ifdef ENABLE_MESSENGER_BLOCK
		std::map<keyT, std::set<keyT> >	m_BlockRelation;
		std::map<keyT, std::set<keyT> >	m_InverseBlockRelation;
		std::set<DWORD>			m_set_requestToBlockAdd;
		#endif
};

#endif
