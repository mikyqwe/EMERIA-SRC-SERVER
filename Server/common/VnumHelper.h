#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__




class CItemVnumHelper
{
public:

	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }


	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum; }


	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }


	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }


	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }
#ifdef SYSTEM_PDA
	static	const bool	IsSoulStone(DWORD vnum)				{ return 50513 == vnum; }
#endif

#ifdef __EXTENDED_BLEND__
	/// Extended Blend
	static const bool IsExtendedBlend(DWORD vnum)
	{
		switch (vnum)
		{
			// NORMAL_DEWS
			case 50821:
			case 50822:
			case 50823:
			case 50824:
			case 50825:
			case 50826:
			case 51002: // Energy Cristal
			// END_OF_NORMAL_DEWS

			// temp_DEWS
			case 850821:
			case 850822:
			case 850823:
			case 850824:
			case 850825:

			case 27863:
			case 27864:
			case 27865:
			case 27866:
			case 27867:
			case 27868:
			case 27869:
			case 27870:
			case 27871:
			case 27872:
			case 27873:
			case 27874:
			case 27875:
			case 27876:
			case 27877:
			case 27878:
			
			// INFINITE_DEWS
			case 950821:
			case 950822:
			case 950823:
			case 950824:
			case 950825:
			case 950826:
			case 951002: // Energy Cristal
			// END_OF_INFINITE_DEWS

			// DRAGON_GOD_MEDALS
			case 939017:
			case 939018:
			case 939019:
			case 939020:
			// END_OF_DRAGON_GOD_MEDALS

			// CRITICAL_AND_PENETRATION
			case 939024:
			case 939025:
			// END_OF_CRITICAL_AND_PENETRATION

			// ATTACK_AND_MOVE_SPEED
			case 927209:
			case 927212:
			// END_OF_ATTACK_AND_MOVE_SPEED
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

	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }

	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }


	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }


	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__
//martysama0134's 2022
