#include "../common/service.h"
#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__

/**
	�̹� �����ϰų� ������ �߰��� ������, �� ���� �ҽ����� �ĺ��� �� ����� ���
	�ĺ���(����=VNum)�� �ϵ��ڵ��ϴ� ������� �Ǿ��־ �������� �ſ� �������µ�

	�����δ� �ҽ��� ���� � ������(Ȥ�� ��)���� �� �� �ְ� ���ڴ� ��ö���� �������� �߰�.

		* �� ������ ������ ���������� ����Ǵµ� PCH�� ������ �ٲ� ������ ��ü ������ �ؾ��ϴ�
		�ϴ��� �ʿ��� cpp���Ͽ��� include �ؼ� ������ ����.

		* cpp���� �����ϸ� ������ ~ ��ũ�ؾ��ϴ� �׳� common�� ����� �־���. (game, db������Ʈ �� �� ��� ����)

	@date	2011. 8. 29.
*/


class CItemVnumHelper
{
public:
	/// ���� DVD�� �һ��� ��ȯ��
	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }		// NOTE: �һ��� ��ȯ �������� 53001 ������ mob-vnum�� 34001 �Դϴ�.

	/// �󸶴� �̺�Ʈ �ʽ´��� ���� (������ �󸶴� �̺�Ʈ�� Ư�� �������̾����� ������ ���� �������� ��Ȱ���ؼ� ��� ���ٰ� ��)
	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum; }

	/// �ҷ��� ���� (������ �ʽ´��� ������ ����)
	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }

	/// ũ�������� �ູ�� ����
	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }

	/// �߷�Ÿ�� ����� �Ҵ�Ʈ
	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }
#ifdef ENABLE_SYSTEM_RUNE
	// effecte rune +9
	static	const bool	IsRunaWhite(DWORD vnum)		{ return 55044 == vnum; }
	static	const bool	IsRunaYellow(DWORD vnum)	{ return 55052 == vnum; }
	static	const bool	IsRunaBlack(DWORD vnum)		{ return 55064 == vnum; }
	static	const bool	IsRunaGreen(DWORD vnum)		{ return 55056 == vnum; }
	static	const bool	IsRunaRed(DWORD vnum)		{ return 55048 == vnum; }
	static	const bool	IsRunaBlue(DWORD vnum)		{ return 55060 == vnum; }
#endif	
	
#ifdef SYSTEM_PDA
	static	const bool	IsSoulStone(DWORD vnum)				{ return 50513 == vnum; }
#endif
#ifdef __EXTENDED_BLEND_AFFECT__
	/// Extended Blend
	static	const bool	IsExtendedBlend(DWORD vnum)
	{
		switch (vnum)
		{
			case 50821:
			case 50822:
			case 50823:
			case 50824:
			case 50825:
			case 50826:
			case 51002:
			case 950821:
			case 950822:
			case 950823:
			case 950824:
			case 950825:
			case 950826:
			case 951002:
				return true;
			default:
				return false;
		}
	}
#endif
};

class CMobVnumHelper
{
public:
	/// ���� DVD�� �һ��� �� ��ȣ
	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }
	/// PetSystem�� �����ϴ� ���ΰ�?
	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }

	/// 2011�� ũ�������� �̺�Ʈ�� �� (�Ʊ� ����)
	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }

	/// �󸶴� �̺�Ʈ ����� �渶(20119) .. �ҷ��� �̺�Ʈ�� �󸶴� �渶 Ŭ��(������ ����, 20219)
	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__
