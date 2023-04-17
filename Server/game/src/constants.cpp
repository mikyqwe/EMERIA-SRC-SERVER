#include "stdafx.h"
#include "char.h"
#include "../../common/CommonDefines.h"
TJobInitialPoints JobInitialPoints[JOB_MAX_NUM] =
/*
   {
   int st, ht, dx, iq;
   int max_hp, max_sp;
   int hp_per_ht, sp_per_iq;
   int hp_per_lv_begin, hp_per_lv_end;
   int sp_per_lv_begin, sp_per_lv_end;
   int max_stamina;
   int stamina_per_con;
   int stamina_per_lv_begin, stamina_per_lv_end;
   }
 */
{
	{   6,  4,  3,  3,  600,   200,     40,    20,    36, 44,     18, 22,     800,      5,      1, 3  }, // JOB_WARRIOR  16
	{   4,  3,  6,  3,  650,   200,     40,    20,    36, 44,     18, 22,     800,      5,      1, 3  }, // JOB_ASSASSIN 16
	{   5,  3,  3,  5,  650,   200,     40,    20,    36, 44,     18, 22,     800,      5,      1, 3  }, // JOB_SURA	 16
	{   3,  4,  3,  6,  700,   200,     40,    20,    36, 44,     18, 22,     800,      5,      1, 3  },  // JOB_SHAMANa  16
#ifdef ENABLE_WOLFMAN_CHARACTER
	{   2,  6,  6,  2,  600,   200,     40,    20,    36, 44,     18, 22,     800,      5,      1, 3  },
#endif
};

#ifdef M2S_BIO_SYSTEM
const std::vector<TBioTable> BioMissions = {
	// id	lvl				items		   count	%		  time				 bonus
	  {0,	30,		{30006,30220,50109},	10,		80,		0,			{54, 50}},
	  {1,	40,		{30047,30221,50109},	15,		80,		0,			{1, 3000}},
	  {2,	50,		{30015,30222,50113},	20,		80,		0,			{15, 7}},
	  {3,	60,		{30050,30223,50113},	20,		70,		0,			{16, 7}},
	  {4,	70,		{30165,30224,50113},	25,		70,		0,			{53, 120}},
	  {5,	80,		{30166,30225,50113},	25,		70,		0,			{115, 5}},
	  {6,	85,		{30167,30226,50114},	35,		60,		0,		{63, 15}},
	  {7,	90,		{30168,30227,50114},	100,	50,		0,		{73, 5}},
	  {8,	94,		{30193,30252,50114},	50,		50,		0,		{71, 5}},
	  {9,	100,	{30195,31043,50110},	50,		50,		0,		{72, 5}},
	  {10,	110,	{30196,30251,50110},	35,		50,		0,		{74, 5}},
	  {11,	120,	{53251,53252,50110},	5,		40,		0,		{17, 10}},
};

#endif


const TMobRankStat MobRankStats[MOB_RANK_MAX_NUM] =
/*
   {
   int         iGoldPercent;
   }
 */
{
	{  20,  }, // MOB_RANK_PAWN,
	{  20,  }, // MOB_RANK_S_PAWN,
	{  25,  }, // MOB_RANK_KNIGHT,
	{  30,  }, // MOB_RANK_S_KNIGHT,
	{  50,  }, // MOB_RANK_BOSS,
	{ 100,  }  // MOB_RANK_KING,
};

TBattleTypeStat BattleTypeStats[BATTLE_TYPE_MAX_NUM] =
/*
   {
   int         AttGradeBias;
   int         DefGradeBias;
   int         MagicAttGradeBias;
   int         MagicDefGradeBias;
   }
 */
{
	{	  0,	  0,	  0,	-10	}, // BATTLE_TYPE_MELEE,
	{	 10,	-20,	-10,	-15	}, // BATTLE_TYPE_RANGE,
	{	 -5,	 -5,	 10,	 10	}, // BATTLE_TYPE_MAGIC,
	{	  0,	  0,	  0,	  0	}, // BATTLE_TYPE_SPECIAL,
	{	 10,	-10,	  0,	-15	}, // BATTLE_TYPE_POWER,
	{	-10,	 10,	-10,	  0	}, // BATTLE_TYPE_TANKER,
	{	 20,	-20,	  0,	-10	}, // BATTLE_TYPE_SUPER_POWER,
	{	-20,	 20,	-10,	  0	}, // BATTLE_TYPE_SUPER_TANKER,
};

const DWORD * exp_table = NULL;


#ifdef NEW_PET_SYSTEM
DWORD * exppet_table = NULL;
//const DWORD pets = 4;
const DWORD Pet_Table[10][2] =
{
	{ 55701, 34041 },
	{ 55702, 34045 },
	{ 55703, 34049 },
	{ 55704, 34053 },
	{ 55705, 34008 },
	{ 55706, 34064 },
	{ 55707, 34073 },
	{ 55708, 34075 },
	{ 55709, 34080 },
	{ 55716, 34038 }
};

const DWORD Pet_Skill_Table[18][23] =
{
	//Level Like Index ->> Bonus Value --> typeSkill --> CoolDown -->> Percentage
	{ 78, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12 },
	{ 80, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12 },
	{ 79, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12 },
	{ 81, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12 },
	{ 93, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12 }, //5
	{ 64, 0, 0, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 10 },//default is 53, not 64
	{ 98, 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11, 12 }, //default is 97 (not worked)
	{ 9, 0, 0, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 16, 17, 17, 17 },
	{ 16, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 8, 8, 9, 9 },
	{ 4, 1, 480, 17, 68, 153, 272, 425, 612, 833, 1088, 1377, 1700, 2057, 2448, 2873, 3332, 3825, 4352, 4913, 5508, 6137, 6800 }, //10
	{ 23, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5 },
	{ 24, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 6, 6 },
	{ 27, 0, 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11 },
	{ 28, 0, 0, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7 },
	{ 44, 0, 0, 5, 10, 15, 20, 25, 31, 36, 41, 46, 51, 56, 61, 67, 72, 77, 82, 87, 92, 97, 103 }, //15
	{ 52, 0, 0, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6 },
	{ 11, 1, 600, 2, 4, 6, 7, 8, 9, 11, 12, 13, 15, 16, 17, 18, 20, 21, 22, 23, 25, 26, 27 },
	{ 6, 1, 480, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }


};

const DWORD Pet_SKill_TablePerc[3][20] =
{
	{ 2, 5, 8, 11, 13, 16, 19, 22, 24, 27, 30, 33, 35, 38, 41, 44, 46, 49, 52, 55 },
	{ 2, 5, 7, 10, 12, 15, 17, 20, 23, 25, 27, 30, 32, 35, 37, 40, 42, 45, 47, 50 },
	{ 2, 5, 8, 11, 13, 16, 19, 22, 25, 27, 30, 33, 36, 38, 41, 44, 47, 50, 52, 55 }
};

DWORD exppet_table_common[121];
#endif


const DWORD exp_table_common[PLAYER_MAX_LEVEL_CONST + 1] =
{
	0,	//	0
	300,
	800,
	1500,
	2500,
	4300,
	7200,
	11000,
	17000,
	24000,
	33000,	//	10
	43000,
	58000,
	76000,
	100000,
	130000,
	169000,
	219000,
	283000,
	365000,
	472000,	//	20
	610000,
	705000,
	813000,
	937000,
	1077000,
	1237000,
	1418000,
	1624000,
	1857000,
	2122000,	//	30
	2421000,
	2761000,
	3145000,
	3580000,
	4073000,
	4632000,
	5194000,
	5717000,
	6264000,
	6837000,	//	40
	7600000,
	8274000,
	8990000,
	9753000,
	10560000,
	11410000,
	12320000,
	13270000,
	14280000,
	15340000,	//	50
	16870000,
	18960000,
	19980000,
	21420000,
	22930000,
	24530000,
	26200000,
	27960000,
	29800000,
	32780000,	//	60
	36060000,
	39670000,
	43640000,
	48000000,
	52800000,
	58080000,
	63890000,
	70280000,
	77310000,
	85040000,	//	70
	93540000,
	102900000,
	113200000,
	124500000,
	137000000,
	150700000,
	165700000,
	236990000,
	260650000,
	286780000,	//	80
	315380000,
	346970000,
	381680000,
	419770000,
	461760000,
	508040000,
	558740000,
	614640000,
	676130000,
	743730000,	//	90
	1041222000,
	1145344200,
	1259878620,
	1385866482,
	1524453130,
	1676898443,
	1844588288,
	2029047116,
	2050000000,
	2150000000u,	//	100
	2210000000u,
	2250000000u,
	2280000000u,
	2310000000u,
	2330000000u,	//	105
	2350000000u,
	2370000000u,
	2390000000u,
	2400000000u,
	2410000000u,	//	110
	2420000000u,
	2430000000u,
	2440000000u,
	2450000000u,
	2460000000u,	//	115
	2470000000u,
	2480000000u,
	2490000000u,
	2490000000u,
	2500000000u,	//	120
	// extra
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 130
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 140
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 150
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 160
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 170
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 180
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 190
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 200
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 210
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 220
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 230
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 240
	2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u,2500000000u, // 250
};

const int * aiPercentByDeltaLev = NULL;
const int * aiPercentByDeltaLevForBoss = NULL;


const int aiPercentByDeltaLevForBoss_euckr[MAX_EXP_DELTA_OF_LEV] =
{
	1,      // -15  0
	3,          // -14  1
	5,          // -13  2
	7,          // -12  3
	15,         // -11  4
	30,         // -10  5
	60,         // -9   6
	90,         // -8   7
	91,         // -7   8
	92,         // -6   9
	93,         // -5   10
	94,         // -4   11
	95,         // -3   12
	97,         // -2   13
	99,         // -1   14
	100,        // 0    15
	105,        // 1    16
	110,        // 2    17
	115,        // 3    18
	120,        // 4    19
	125,        // 5    20
	130,        // 6    21
	135,        // 7    22
	140,        // 8    23
	145,        // 9    24
	150,        // 10   25
	155,        // 11   26
	160,        // 12   27
	165,        // 13   28
	170,        // 14   29
	180         // 15   30
};



const int aiPercentByDeltaLev_euckr[MAX_EXP_DELTA_OF_LEV] =
{
	1,  //  -15 0
	5,  //  -14 1
	10, //  -13 2
	20, //  -12 3
	30, //  -11 4
	50, //  -10 5
	70, //  -9  6
	80, //  -8  7
	85, //  -7  8
	90, //  -6  9
	92, //  -5  10
	94, //  -4  11
	96, //  -3  12
	98, //  -2  13
	100,    //  -1  14
	100,    //  0   15
	105,    //  1   16
	110,    //  2   17
	115,    //  3   18
	120,    //  4   19
	125,    //  5   20
	130,    //  6   21
	135,    //  7   22
	140,    //  8   23
	145,    //  9   24
	150,    //  10  25
	155,    //  11  26
	160,    //  12  27
	165,    //  13  28
	170,    //  14  29
	180,    //  15  30
};

const DWORD party_exp_distribute_table[PLAYER_EXP_TABLE_MAX + 1] =
{
	0,
	10,		10,		10,		10,		15,		15,		20,		25,		30,		40,		// 1 - 10
	50,		60,		80,		100,	120,	140,	160,	184,	210,	240,	// 11 - 20
	270,	300,	330,	360,	390,	420,	450,	480,	510,	550,	// 21 - 30
	600,	640,	700,	760,	820,	880,	940,	1000,	1100,	1180,	// 31 - 40
	1260,	1320,	1380,	1440,	1500,	1560,	1620,	1680,	1740,	1800,	// 41 - 50
	1860,	1920,	2000,	2100,	2200,	2300,	2450,	2600,	2750,	2900,	// 51 - 60
	3050,	3200,	3350,	3500,	3650,	3800,	3950,	4100,	4250,	4400,	// 61 - 70
	4600,	4800,	5000,	5200,	5400,	5600,	5800,	6000,	6200,	6400,	// 71 - 80
	6600,	6900,	7100,	7300,	7600,	7800,	8000,	8300,	8500,	8800,	// 81 - 90
	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	9000,	// 91 - 100
	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	10000,	// 101 - 110
	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	12000,	// 111 - 120
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 130
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 140
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 150
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 160
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 170
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 180
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 190
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 200
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 210
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 220
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 230
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 240
	// 14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	14000,	// 250
};

Coord aArroundCoords[ARROUND_COORD_MAX_NUM] =
{
	{	     0,	      0	    },
	{        0,      50     },
	{       35,     35      },
	{       50,     -0      },
	{       35,     -35     },
	{       0,      -50     },
	{       -35,    -35     },
	{       -50,    0       },
	{       -35,    35      },
	{       0,      100     },
	{       71,     71      },
	{       100,    -0      },
	{       71,     -71     },
	{       0,      -100    },
	{       -71,    -71     },
	{       -100,   0       },
	{       -71,    71      },
	{       0,      150     },
	{       106,    106     },
	{       150,    -0      },
	{       106,    -106    },
	{       0,      -150    },
	{       -106,   -106    },
	{       -150,   0       },
	{       -106,   106     },
	{       0,      200     },
	{       141,    141     },
	{       200,    -0      },
	{       141,    -141    },
	{       0,      -200    },
	{       -141,   -141    },
	{       -200,   0       },
	{       -141,   141     },
	{       0,      250     },
	{       177,    177     },
	{       250,    -0      },
	{       177,    -177    },
	{       0,      -250    },
	{       -177,   -177    },
	{       -250,   0       },
	{       -177,   177     },
	{       0,      300     },
	{       212,    212     },
	{       300,    -0      },
	{       212,    -212    },
	{       0,      -300    },
	{       -212,   -212    },
	{       -300,   0       },
	{       -212,   212     },
	{       0,      350     },
	{       247,    247     },
	{       350,    -0      },
	{       247,    -247    },
	{       0,      -350    },
	{       -247,   -247    },
	{       -350,   0       },
	{       -247,   247     },
	{       0,      400     },
	{       283,    283     },
	{       400,    -0      },
	{       283,    -283    },
	{       0,      -400    },
	{       -283,   -283    },
	{       -400,   0       },
	{       -283,   283     },
	{       0,      450     },
	{       318,    318     },
	{       450,    -0      },
	{       318,    -318    },
	{       0,      -450    },
	{       -318,   -318    },
	{       -450,   0       },
	{       -318,   318     },
	{       0,      500     },
	{       354,    354     },
	{       500,    -0      },
	{       354,    -354    },
	{       0,      -500    },
	{       -354,   -354    },
	{       -500,   0       },
	{       -354,   354     },
	{       0,      550     },
	{       389,    389     },
	{       550,    -0      },
	{       389,    -389    },
	{       0,      -550    },
	{       -389,   -389    },
	{       -550,   0       },
	{       -389,   389     },
	{       0,      600     },
	{       424,    424     },
	{       600,    -0      },
	{       424,    -424    },
	{       0,      -600    },
	{       -424,   -424    },
	{       -600,   0       },
	{       -424,   424     },
	{       0,      650     },
	{       460,    460     },
	{       650,    -0      },
	{       460,    -460    },
	{       0,      -650    },
	{       -460,   -460    },
	{       -650,   0       },
	{       -460,   460     },
	{       0,      700     },
	{       495,    495     },
	{       700,    -0      },
	{       495,    -495    },
	{       0,      -700    },
	{       -495,   -495    },
	{       -700,   0       },
	{       -495,   495     },
	{       0,      750     },
	{       530,    530     },
	{       750,    -0      },
	{       530,    -530    },
	{       0,      -750    },
	{       -530,   -530    },
	{       -750,   0       },
	{       -530,   530     },
	{       0,      800     },
	{       566,    566     },
	{       800,    -0      },
	{       566,    -566    },
	{       0,      -800    },
	{       -566,   -566    },
	{       -800,   0       },
	{       -566,   566     },
	{       0,      850     },
	{       601,    601     },
	{       850,    -0      },
	{       601,    -601    },
	{       0,      -850    },
	{       -601,   -601    },
	{       -850,   0       },
	{       -601,   601     },
	{       0,      900     },
	{       636,    636     },
	{       900,    -0      },
	{       636,    -636    },
	{       0,      -900    },
	{       -636,   -636    },
	{       -900,   0       },
	{       -636,   636     },
	{       0,      950     },
	{       672,    672     },
	{       950,    -0      },
	{       672,    -672    },
	{       0,      -950    },
	{       -672,   -672    },
	{       -950,   0       },
	{       -672,   672     },
	{       0,      1000    },
	{       707,    707     },
	{       1000,   -0      },
	{       707,    -707    },
	{       0,      -1000   },
	{       -707,   -707    },
	{       -1000,  0       },
	{       -707,   707     },
};

const DWORD guild_exp_table[GUILD_MAX_LEVEL+1] =
{
	0,
	15000UL,
	45000UL,
	90000UL,
	160000UL,
	235000UL,
	325000UL,
	430000UL,
	550000UL,
	685000UL,
	835000UL,
	1000000UL,
	1500000UL,
	2100000UL,
	2800000UL,
	3600000UL,
	4500000UL,
	6500000UL,
	8000000UL,
	10000000UL,
	42000000UL
};

const DWORD guild_exp_table2[GUILD_MAX_LEVEL+1] =
{
	0,
	6000UL,
	18000UL,
	36000UL,
	64000UL,
	94000UL,
	130000UL,
	172000UL,
	220000UL,
	274000UL,
	334000UL,
	400000UL,
	600000UL,
	840000UL,
	1120000UL,
	1440000UL,
	1800000UL,
	2600000UL,
	3200000UL,
	4000000UL,
	16800000UL
};

const int aiMobEnchantApplyIdx[MOB_ENCHANTS_MAX_NUM] =
{
	APPLY_CURSE_PCT,
	APPLY_SLOW_PCT,
	APPLY_POISON_PCT,
	APPLY_STUN_PCT,
	APPLY_CRITICAL_PCT,
	APPLY_PENETRATE_PCT,
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	APPLY_BLEEDING_PCT,
#endif
};

const int aiMobResistsApplyIdx[MOB_RESISTS_MAX_NUM] =
{
	APPLY_RESIST_SWORD,
	APPLY_RESIST_TWOHAND,
	APPLY_RESIST_DAGGER,
	APPLY_RESIST_BELL,
	APPLY_RESIST_FAN,
	APPLY_RESIST_BOW,
	APPLY_RESIST_FIRE,
	APPLY_RESIST_ELEC,
	APPLY_RESIST_MAGIC,
	APPLY_RESIST_WIND,
	APPLY_POISON_REDUCE,
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_CLAW_AS_DAGGER)
	APPLY_RESIST_CLAW,
#endif
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	APPLY_BLEEDING_REDUCE,
#endif
};

const int aiSocketPercentByQty[5][4] =
{
	{  0,  0,  0,  0 },
	{  3,  0,  0,  0 },
	{ 10,  1,  0,  0 },
	{ 15, 10,  1,  0 },
	{ 20, 15, 10,  1 }
};

const int aiWeaponSocketQty[WEAPON_NUM_TYPES] =
{
	3, // WEAPON_SWORD,
	3, // WEAPON_DAGGER,
	3, // WEAPON_BOW,
	3, // WEAPON_TWO_HANDED,
	3, // WEAPON_BELL,
	3, // WEAPON_FAN,
	0, // WEAPON_ARROW,
	0, // WEAPON_MOUNT_SPEAR
#ifdef ENABLE_WOLFMAN_CHARACTER
	3, // WEAPON_CLAW
#endif
};

const int aiArmorSocketQty[ARMOR_NUM_TYPES] =
{
	3, // ARMOR_BODY,
	1, // ARMOR_HEAD,
	1, // ARMOR_SHIELD,
	0, // ARMOR_WRIST,
	0, // ARMOR_FOOTS,
	0  // ARMOR_ACCESSORY
};

TItemAttrMap g_map_itemAttr;
TItemAttrMap g_map_itemRare;

const TApplyInfo aApplyInfo[MAX_APPLY_NUM] =
/*
{
   DWORD dwPointType;
}
 */
{
	// Point Type
	{ POINT_NONE,			},   // APPLY_NONE,		0
	{ POINT_MAX_HP,		        },   // APPLY_MAX_HP,		1
	{ POINT_MAX_SP,		        },   // APPLY_MAX_SP,		2
	{ POINT_HT,			        },   // APPLY_CON,		3
	{ POINT_IQ,			        },   // APPLY_INT,		4
	{ POINT_ST,			        },   // APPLY_STR,		5
	{ POINT_DX,			        },   // APPLY_DEX,		6
	{ POINT_ATT_SPEED,		        },   // APPLY_ATT_SPEED,	7
	{ POINT_MOV_SPEED,		        },   // APPLY_MOV_SPEED,	8
	{ POINT_CASTING_SPEED,	        },   // APPLY_CAST_SPEED,	9
	{ POINT_HP_REGEN,			},   // APPLY_HP_REGEN,		10
	{ POINT_SP_REGEN,			},   // APPLY_SP_REGEN,		11
	{ POINT_POISON_PCT,		        },   // APPLY_POISON_PCT,	12
	{ POINT_STUN_PCT,		        },   // APPLY_STUN_PCT,		13
	{ POINT_SLOW_PCT,		        },   // APPLY_SLOW_PCT,		14
	{ POINT_CRITICAL_PCT,		},   // APPLY_CRITICAL_PCT,	15
	{ POINT_PENETRATE_PCT,	        },   // APPLY_PENETRATE_PCT,	16
	{ POINT_ATTBONUS_HUMAN,	        },   // APPLY_ATTBONUS_HUMAN,	17
	{ POINT_ATTBONUS_ANIMAL,	        },   // APPLY_ATTBONUS_ANIMAL,	18
	{ POINT_ATTBONUS_ORC,		},   // APPLY_ATTBONUS_ORC,	19
	{ POINT_ATTBONUS_MILGYO,	        },   // APPLY_ATTBONUS_MILGYO,	20
	{ POINT_ATTBONUS_UNDEAD,	        },   // APPLY_ATTBONUS_UNDEAD,	21
	{ POINT_ATTBONUS_DEVIL,	        },   // APPLY_ATTBONUS_DEVIL,	22
	{ POINT_STEAL_HP,		        },   // APPLY_STEAL_HP,		23
	{ POINT_STEAL_SP,		        },   // APPLY_STEAL_SP,		24
	{ POINT_MANA_BURN_PCT,	        },   // APPLY_MANA_BURN_PCT,	25
	{ POINT_DAMAGE_SP_RECOVER,	        },   // APPLY_DAMAGE_SP_RECOVER,26
	{ POINT_BLOCK,		        },   // APPLY_BLOCK,		27
	{ POINT_DODGE,		        },   // APPLY_DODGE,		28
	{ POINT_RESIST_SWORD,		},   // APPLY_RESIST_SWORD,	29
	{ POINT_RESIST_TWOHAND,	        },   // APPLY_RESIST_TWOHAND,	30
	{ POINT_RESIST_DAGGER,	        },   // APPLY_RESIST_DAGGER,	31
	{ POINT_RESIST_BELL,		},   // APPLY_RESIST_BELL,	32
	{ POINT_RESIST_FAN,		        },   // APPLY_RESIST_FAN,	33
	{ POINT_RESIST_BOW,		        },   // APPLY_RESIST_BOW,	34
	{ POINT_RESIST_FIRE,		},   // APPLY_RESIST_FIRE,	35
	{ POINT_RESIST_ELEC,		},   // APPLY_RESIST_ELEC,	36
	{ POINT_RESIST_MAGIC,		},   // APPLY_RESIST_MAGIC,	37
	{ POINT_RESIST_WIND,		},   // APPLY_RESIST_WIND,	38
	{ POINT_REFLECT_MELEE,	        },   // APPLY_REFLECT_MELEE,	39
	{ POINT_REFLECT_CURSE,	        },   // APPLY_REFLECT_CURSE,	40
	{ POINT_POISON_REDUCE,	        },   // APPLY_POISON_REDUCE,	41
	{ POINT_KILL_SP_RECOVER,	        },   // APPLY_KILL_SP_RECOVER,	42
	{ POINT_EXP_DOUBLE_BONUS,	        },   // APPLY_EXP_DOUBLE_BONUS,	43
	{ POINT_GOLD_DOUBLE_BONUS,	        },   // APPLY_GOLD_DOUBLE_BONUS,44
	{ POINT_ITEM_DROP_BONUS,	        },   // APPLY_ITEM_DROP_BONUS,	45
	{ POINT_POTION_BONUS,		},   // APPLY_POTION_BONUS,	46
	{ POINT_KILL_HP_RECOVERY,	        },   // APPLY_KILL_HP_RECOVER,	47
	{ POINT_IMMUNE_STUN,		},   // APPLY_IMMUNE_STUN,	48
	{ POINT_IMMUNE_SLOW,		},   // APPLY_IMMUNE_SLOW,	49
	{ POINT_IMMUNE_FALL,		},   // APPLY_IMMUNE_FALL,	50
	{ POINT_NONE,			},   // APPLY_SKILL,		51
	{ POINT_BOW_DISTANCE,		},   // APPLY_BOW_DISTANCE,	52
	{ POINT_ATT_GRADE_BONUS,	        },   // APPLY_ATT_GRADE,	53
	{ POINT_DEF_GRADE_BONUS,	        },   // APPLY_DEF_GRADE,	54
	{ POINT_MAGIC_ATT_GRADE_BONUS,      },   // APPLY_MAGIC_ATT_GRADE,	55
	{ POINT_MAGIC_DEF_GRADE_BONUS,      },   // APPLY_MAGIC_DEF_GRADE,	56
	{ POINT_CURSE_PCT,			},   // APPLY_CURSE_PCT,	57
	{ POINT_MAX_STAMINA			},   // APPLY_MAX_STAMINA	58
	{ POINT_ATTBONUS_WARRIOR		},   // APPLY_ATTBONUS_WARRIOR  59
	{ POINT_ATTBONUS_ASSASSIN		},   // APPLY_ATTBONUS_ASSASSIN 60
	{ POINT_ATTBONUS_SURA		},   // APPLY_ATTBONUS_SURA     61
	{ POINT_ATTBONUS_SHAMAN		},   // APPLY_ATTBONUS_SHAMAN   62
	{ POINT_ATTBONUS_MONSTER		},   //	APPLY_ATTBONUS_MONSTER  63
	{ POINT_ATT_BONUS		},   // 64 // APPLY_MALL_ATTBONUS
	{ POINT_MALL_DEFBONUS		},   // 65
	{ POINT_MALL_EXPBONUS		},   // 66 APPLY_MALL_EXPBONUS
	{ POINT_MALL_ITEMBONUS		},   // 67
	{ POINT_MALL_GOLDBONUS		},   // 68
	{ POINT_MAX_HP_PCT			},		// 69
	{ POINT_MAX_SP_PCT			},		// 70
	{ POINT_SKILL_DAMAGE_BONUS		},	// 71
	{ POINT_NORMAL_HIT_DAMAGE_BONUS	},	// 72

	// DEFEND_BONUS_ATTRIBUTES
	{ POINT_SKILL_DEFEND_BONUS		},	// 73
	{ POINT_NORMAL_HIT_DEFEND_BONUS	},	// 74
	// END_OF_DEFEND_BONUS_ATTRIBUTES

	// PC_BANG_ITEM_ADD
	{ POINT_PC_BANG_EXP_BONUS	},		// 75
	{ POINT_PC_BANG_DROP_BONUS	},		// 76
	// END_PC_BANG_ITEM_ADD

	{ POINT_NONE,		},

	{ POINT_RESIST_WARRIOR,		},
	{ POINT_RESIST_ASSASSIN,	},
	{ POINT_RESIST_SURA,		},
	{ POINT_RESIST_SHAMAN,		},
	{ POINT_ENERGY				},
	{ POINT_DEF_GRADE			},
	{ POINT_COSTUME_ATTR_BONUS	},
	{ POINT_MAGIC_ATT_BONUS_PER },
	{ POINT_MELEE_MAGIC_ATT_BONUS_PER		},			// 86 APPLY_MELEE_MAGIC_ATTBONUS_PER
	{ POINT_RESIST_ICE,			},   // APPLY_RESIST_ICE,	87
	{ POINT_RESIST_EARTH,		},   // APPLY_RESIST_EARTH,	88
	{ POINT_RESIST_DARK,		},   // APPLY_RESIST_DARK,	89
	{ POINT_RESIST_CRITICAL,		},   // APPLY_ANTI_CRITICAL_PCT,	90
	{ POINT_RESIST_PENETRATE,		},   // APPLY_ANTI_PENETRATE_PCT,	91

#ifdef ENABLE_WOLFMAN_CHARACTER
	{ POINT_BLEEDING_REDUCE,		},	// APPLY_BLEEDING_REDUCE, 		92
	{ POINT_BLEEDING_PCT,			},	// APPLY_BLEEDING_PCT, 			93
	{ POINT_ATTBONUS_WOLFMAN,		},
	{ POINT_RESIST_WOLFMAN,			},
	{ POINT_RESIST_CLAW,			},
#else
	{ POINT_NONE,					},	// APPLY_BLEEDING_REDUCE, 		92
	{ POINT_NONE,					},	// APPLY_BLEEDING_PCT, 			93
	{ POINT_NONE,					},
	{ POINT_NONE,					},
	{ POINT_NONE,					},
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ POINT_ACCEDRAIN_RATE,			},	// APPLY_ACCEDRAIN_RATE,		97
#else
	{ POINT_NONE,			},	// APPLY_ACCEDRAIN_RATE,		97
#endif

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	{ POINT_RESIST_MAGIC_REDUCTION,	},	// APPLY_RESIST_MAGIC_REDUCTION,98
#else
	{ POINT_NONE,					},	// APPLY_RESIST_MAGIC_REDUCTION,98
#endif
};

const int aiItemMagicAttributePercentHigh[ITEM_ATTRIBUTE_MAX_LEVEL] =
{
	//25, 25, 40, 8, 2,
	30, 40, 20, 8, 2
};

const int aiItemMagicAttributePercentLow[ITEM_ATTRIBUTE_MAX_LEVEL] =
{
	//45, 25, 20, 10, 0,
	50, 40, 10, 0, 0
};

// ADD_ITEM_ATTRIBUTE
const int aiItemAttributeAddPercent[ITEM_ATTRIBUTE_MAX_NUM] =
{
	100, 80, 60, 50, 30, 0, 0,
};
// END_OF_ADD_ITEM_ATTRIBUTE

const int aiExpLossPercents[PLAYER_EXP_TABLE_MAX + 1] =
{
	0,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 4, // 1 - 10
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 11 - 20
	4, 4, 4, 4, 4, 4, 4, 3, 3, 3, // 21 - 30
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 31 - 40
	3, 3, 3, 3, 2, 2, 2, 2, 2, 2, // 41 - 50
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 51 - 60
	2, 2, 1, 1, 1, 1, 1, 1, 1, 1, // 61 - 70
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 71 - 80
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 81 - 90
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 91 - 100
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 101 - 110
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 111 - 120
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 130
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 140
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 150
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 160
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 170
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 180
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 190
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 200
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 210
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 220
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 230
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 240
	// 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 250
};

const int aiSkillBookCountForLevelUp[10] =
{
	3, 3, 3, 3, 3, 4, 4, 5, 5, 6
};

// ADD_GRANDMASTER_SKILL
const int aiGrandMasterSkillBookCountForLevelUp[10] =
{
	3, 3, 5, 5, 7, 7, 10, 10, 10, 20,
};

const int aiGrandMasterSkillBookMinCount[10] =
{
//	1, 1, 3, 5, 10, 15, 20, 30, 40, 50,
//	3, 3, 5, 5, 10, 10, 15, 15, 20, 30
	1, 1, 1, 2,  2,  3,  3,  4,  5,  6
};

const int aiGrandMasterSkillBookMaxCount[10] =
{
//	6, 15, 30, 45, 60, 80, 100, 120, 160, 200,
//	6, 10, 15, 20, 30, 40, 50, 60, 70, 80
	5,  7,  9, 11, 13, 15, 20, 25, 30, 35
};
// END_OF_ADD_GRANDMASTER_SKILL

const int CHN_aiPartyBonusExpPercentByMemberCount[9] =
{
	0, 0, 12, 18, 26, 40, 53, 70, 100
};


// UPGRADE_PARTY_BONUS
const int KOR_aiPartyBonusExpPercentByMemberCount[9] =
{
	0,
	0,
	30, // 66% * 2 - 100
	60, // 53% * 3 - 100
	75, // 44% * 4 - 100
	90, // 38% * 5 - 100
	105, // 34% * 6 - 100
	110, // 30% * 7 - 100
	140, // 30% * 8 - 100
};

const int KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[9] =
{
	0,
	0,
	15*2,
	14*3,
	13*4,
	12*5,
	11*6,
	10*7,
	10*8,
};
// END_OF_UPGRADE_PARTY_BONUS

const int * aiChainLightningCountBySkillLevel = NULL;

const int aiChainLightningCountBySkillLevel_euckr[SKILL_MAX_LEVEL+1] =
{
	0,	// 0
	2,	// 1
	2,	// 2
	2,	// 3
	2,	// 4
	2,	// 5
	2,	// 6
	2,	// 7
	2,	// 8
	3,	// 9
	3,	// 10
	3,	// 11
	3,	// 12
	3,	// 13
	3,	// 14
	3,	// 15
	3,	// 16
	3,	// 17
	3,	// 18
	4,	// 19
	4,	// 20
	4,	// 21
	4,	// 22
	4,	// 23
	5,	// 24
	5,	// 25
	5,	// 26
	5,	// 27
	5,	// 28
	5,	// 29
	5,	// 30
	5,	// 31
	5,	// 32
	5,	// 33
	5,	// 34
	5,	// 35
	5,	// 36
	5,	// 37
	5,	// 38
	5,	// 39
	5,	// 40
};

const SStoneDropInfo aStoneDrop[STONE_INFO_MAX_NUM] =
{
	//  mob		pct	{+0	+1	+2	+3	+4}
	{8005,	60,	{30,	30,	30,	9,	1}	},
	{8006,	60,	{28,	29,	31,	11,	1}	},
	{8007,	60,	{24,	29,	32,	13,	2}	},
	{8008,	60,	{22,	28,	33,	15,	2}	},
	{8009,	60,	{21,	27,	33,	17,	2}	},
	{8010,	60,	{18,	26,	34,	20,	2}	},
	{8011,	60,	{14,	26,	35,	22,	3}	},
	{8012,	60,	{10,	26,	37,	24,	3}	},
	{8013,	60,	{2,	26,	40,	29,	3}	},
	{8014,	60,	{0,	26,	41,	30,	3}	},
};

const char * c_apszEmpireNames[EMPIRE_MAX_NUM] =
{
	"전제국",
	"신수국",
	"천조국",
	"진노국"
};

const char * c_apszPrivNames[MAX_PRIV_NUM] =
{
	"",
	"아이템이 나올 확률",
	"돈이 나올 확률",
	"돈 대박이 나올 확률",
	"경험치 배율",
};

const int aiPolymorphPowerByLevel[SKILL_MAX_LEVEL + 1] =
{
	10,   // 1
	11,   // 2
	11,   // 3
	12,   // 4
	13,   // 5
	13,   // 6
	14,   // 7
	15,   // 8
	16,   // 9
	17,   // 10
	18,   // 11
	19,   // 12
	20,   // 13
	22,   // 14
	23,   // 15
	24,   // 16
	26,   // 17
	27,   // 18
	29,   // 19
	31,   // 20
	33,   // 21
	35,   // 22
	37,   // 23
	39,   // 24
	41,   // 25
	44,   // 26
	46,   // 27
	49,   // 28
	52,   // 29
	55,   // 30
	59,   // 31
	62,   // 32
	66,   // 33
	70,   // 34
	74,   // 35
	79,   // 36
	84,   // 37
	89,   // 38
	94,   // 39
	100,  // 40
};

TGuildWarInfo KOR_aGuildWarInfo[GUILD_WAR_TYPE_MAX_NUM] =
/*
   {
   long lMapIndex;
   int iWarPrice;
   int iWinnerPotionRewardPctToWinner;
   int iLoserPotionRewardPctToWinner;
   int iInitialScore;
   int iEndScore;
   };
 */
{
	{ 0,        0,      0,      0,      0,      0       },
	{ 110,      0,      100,    50,     0,      100     },
	{ 111,      0,      100,    50,     0,      10      },
};

//

//


const int aiAccessorySocketAddPct[ITEM_ACCESSORY_SOCKET_MAX_NUM] =
{
	50, 50, 50
};


const int aiAccessorySocketEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
	0, 10, 20, 40
};


const int aiAccessorySocketDegradeTime[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
	0, 3600 * 24, 3600 * 12, 3600 * 6
};


const int aiAccessorySocketPutPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1] =
{
	90, 80, 70, 0
};
// END_OF_ACCESSORY_REFINE

#include "../../common/length.h"
// from import_item_proto.c
typedef struct SValueName
{
    const char *	c_pszName;
    long		lValue;
} TValueName;

TValueName c_aApplyTypeNames[] =
{
    { "STR",		APPLY_STR		},
    { "DEX",		APPLY_DEX		},
    { "CON",		APPLY_CON		},
    { "INT",		APPLY_INT		},
    { "MAX_HP",		APPLY_MAX_HP		},
    { "MAX_SP",		APPLY_MAX_SP		},
    { "MAX_STAMINA",	APPLY_MAX_STAMINA	},
    { "POISON_REDUCE",	APPLY_POISON_REDUCE	},
    { "EXP_DOUBLE_BONUS", APPLY_EXP_DOUBLE_BONUS },
    { "GOLD_DOUBLE_BONUS", APPLY_GOLD_DOUBLE_BONUS },
    { "ITEM_DROP_BONUS", APPLY_ITEM_DROP_BONUS	},
    { "HP_REGEN",	APPLY_HP_REGEN		},
    { "SP_REGEN",	APPLY_SP_REGEN		},
    { "ATTACK_SPEED",	APPLY_ATT_SPEED		},
    { "MOVE_SPEED",	APPLY_MOV_SPEED		},
    { "CAST_SPEED",	APPLY_CAST_SPEED	},
    { "ATT_BONUS",	APPLY_ATT_GRADE_BONUS	},
    { "DEF_BONUS",	APPLY_DEF_GRADE_BONUS	},
    { "MAGIC_ATT_GRADE",APPLY_MAGIC_ATT_GRADE	},
    { "MAGIC_DEF_GRADE",APPLY_MAGIC_DEF_GRADE	},
    { "SKILL",		APPLY_SKILL		},
    { "ATTBONUS_ANIMAL",APPLY_ATTBONUS_ANIMAL	},
    { "ATTBONUS_UNDEAD",APPLY_ATTBONUS_UNDEAD	},
    { "ATTBONUS_DEVIL", APPLY_ATTBONUS_DEVIL	},
    { "ATTBONUS_HUMAN", APPLY_ATTBONUS_HUMAN	},
    { "ADD_BOW_DISTANCE",APPLY_BOW_DISTANCE	},
    { "DODGE",		APPLY_DODGE		},
    { "BLOCK",		APPLY_BLOCK		},
    { "RESIST_SWORD",	APPLY_RESIST_SWORD	},
    { "RESIST_TWOHAND",	APPLY_RESIST_TWOHAND	},
    { "RESIST_DAGGER",	APPLY_RESIST_DAGGER    },
    { "RESIST_BELL",	APPLY_RESIST_BELL	},
    { "RESIST_FAN",	APPLY_RESIST_FAN	},
    { "RESIST_BOW",	APPLY_RESIST_BOW	},
    { "RESIST_FIRE",	APPLY_RESIST_FIRE	},
    { "RESIST_ELEC",	APPLY_RESIST_ELEC	},
    { "RESIST_MAGIC",	APPLY_RESIST_MAGIC	},
    { "RESIST_WIND",	APPLY_RESIST_WIND	},
	{ "REFLECT_MELEE",	APPLY_REFLECT_MELEE },
	{ "REFLECT_CURSE",	APPLY_REFLECT_CURSE },
    { "RESIST_ICE",		APPLY_RESIST_ICE	},
    { "RESIST_EARTH",	APPLY_RESIST_EARTH	},
    { "RESIST_DARK",	APPLY_RESIST_DARK	},
    { "RESIST_CRITICAL",	APPLY_ANTI_CRITICAL_PCT	},
    { "RESIST_PENETRATE",	APPLY_ANTI_PENETRATE_PCT	},
    { "POISON",		APPLY_POISON_PCT	},
    { "SLOW",		APPLY_SLOW_PCT		},
    { "STUN",		APPLY_STUN_PCT		},
    { "STEAL_HP",	APPLY_STEAL_HP		},
    { "STEAL_SP",	APPLY_STEAL_SP		},
    { "MANA_BURN_PCT",	APPLY_MANA_BURN_PCT	},
    { "CRITICAL",	APPLY_CRITICAL_PCT	},
    { "PENETRATE",	APPLY_PENETRATE_PCT	},
    { "KILL_SP_RECOVER",APPLY_KILL_SP_RECOVER	},
    { "KILL_HP_RECOVER",APPLY_KILL_HP_RECOVER	},
    { "PENETRATE_PCT",	APPLY_PENETRATE_PCT	},
    { "CRITICAL_PCT",	APPLY_CRITICAL_PCT	},
    { "POISON_PCT",	APPLY_POISON_PCT	},
    { "STUN_PCT",	APPLY_STUN_PCT		},
    { "ATT_BONUS_TO_WARRIOR",	APPLY_ATTBONUS_WARRIOR  },
    { "ATT_BONUS_TO_ASSASSIN",	APPLY_ATTBONUS_ASSASSIN },
    { "ATT_BONUS_TO_SURA",	APPLY_ATTBONUS_SURA	    },
    { "ATT_BONUS_TO_SHAMAN",	APPLY_ATTBONUS_SHAMAN   },
    { "ATT_BONUS_TO_MONSTER",	APPLY_ATTBONUS_MONSTER  },
    { "ATT_BONUS_TO_MOB",	APPLY_ATTBONUS_MONSTER  },
    { "MALL_ATTBONUS",	APPLY_MALL_ATTBONUS	},
    { "MALL_EXPBONUS",	APPLY_MALL_EXPBONUS	},
    { "MALL_DEFBONUS",	APPLY_MALL_DEFBONUS	},
    { "MALL_ITEMBONUS",	APPLY_MALL_ITEMBONUS	},
    { "MALL_GOLDBONUS", APPLY_MALL_GOLDBONUS	},
    { "MAX_HP_PCT",	APPLY_MAX_HP_PCT	},
    { "MAX_SP_PCT",	APPLY_MAX_SP_PCT	},
    { "SKILL_DAMAGE_BONUS",	APPLY_SKILL_DAMAGE_BONUS	},
    { "NORMAL_HIT_DAMAGE_BONUS",APPLY_NORMAL_HIT_DAMAGE_BONUS	},
    { "SKILL_DEFEND_BONUS",	APPLY_SKILL_DEFEND_BONUS	},
    { "NORMAL_HIT_DEFEND_BONUS",APPLY_NORMAL_HIT_DEFEND_BONUS	},
    { "PCBANG_EXP_BONUS", APPLY_PC_BANG_EXP_BONUS	},
    { "PCBANG_DROP_BONUS", APPLY_PC_BANG_DROP_BONUS	},

    { "RESIST_WARRIOR",	APPLY_RESIST_WARRIOR},
    { "RESIST_ASSASSIN",	APPLY_RESIST_ASSASSIN},
    { "RESIST_SURA",		APPLY_RESIST_SURA},
    { "RESIST_SHAMAN",	APPLY_RESIST_SHAMAN},
    { "INFINITE_AFFECT_DURATION", 0x1FFFFFFF	},
	{ "ENERGY", APPLY_ENERGY },
	{ "COSTUME_ATTR_BONUS", APPLY_COSTUME_ATTR_BONUS },
	{ "MAGIC_ATTBONUS_PER",	APPLY_MAGIC_ATTBONUS_PER	},
    { "MELEE_MAGIC_ATTBONUS_PER",	APPLY_MELEE_MAGIC_ATTBONUS_PER	},

#ifdef ENABLE_WOLFMAN_CHARACTER
	{ "BLEEDING_REDUCE",APPLY_BLEEDING_REDUCE },
	{ "BLEEDING_PCT",APPLY_BLEEDING_PCT },
	{ "ATT_BONUS_TO_WOLFMAN",APPLY_ATTBONUS_WOLFMAN },
	{ "RESIST_WOLFMAN",APPLY_RESIST_WOLFMAN },
	{ "RESIST_CLAW",APPLY_RESIST_CLAW },
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ "ACCEDRAIN_RATE",APPLY_ACCEDRAIN_RATE },
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	{ "RESIST_MAGIC_REDUCTION",APPLY_RESIST_MAGIC_REDUCTION },
#endif

    { NULL,		0			}
};
// from import_item_proto.c

long FN_get_apply_type(const char *apply_type_string)
{
	TValueName	*value_name;
	for (value_name = c_aApplyTypeNames; value_name->c_pszName; ++value_name)
	{
		if (0==strcasecmp(value_name->c_pszName, apply_type_string))
			return value_name->lValue;
	}
	return 0;
}

#ifdef ENABLE_HUNTING_SYSTEM
const DWORD THuntingMissions[HUNTING_MISSION_COUNT+1][2][2] =
{
	// mob1 count1 , mob2 count2,
	{{0, 0}, {0, 0}}, // None
	{{171, 5}, {172, 3}},		// Lv1
	{{171, 10}, {172, 5}},		// Lv2
	{{171, 20}, {172, 10}},	// Lv3
	{{172, 15}, {173, 5}},		// Lv4
	{{173, 10}, {174, 10}},	// Lv5
	{{174, 20}, {178, 10}},	// Lv6
	{{178, 10}, {175, 5}},		// Lv7
	{{178, 20}, {175, 10}},	// Lv8
	{{175, 15}, {179, 5}},		// Lv9
	{{175, 20}, {179, 10}},	// Lv10
	
	{{179, 10}, {180, 5}},		// Lv11
	{{180, 15}, {176, 10}},	// Lv12
	{{176, 20}, {181, 5}},		// Lv13
	{{181, 15}, {177, 5}},		// Lv14
	{{181, 20}, {177, 10}},	// Lv15
	{{177, 15}, {184, 5}},		// Lv16
	{{177, 20}, {184, 10}},	// Lv17
	{{184, 10}, {182, 10}},	// Lv18
	{{182, 20}, {183, 10}},	// Lv19
	{{183, 20}, {352, 15}},	// Lv20
	
	{{352, 10}, {185, 10}},	// Lv21
	{{185, 25}, {354, 10}},	// Lv22
	{{354, 20}, {451, 40}},	// Lv23
	{{451, 60}, {402, 80}},	// Lv24
	{{551, 20}, {454, 20}},	// Lv25
	{{552, 20}, {456, 10}},	// Lv26
	{{456, 20}, {554, 20}},	// Lv27
	{{651, 20}, {554, 30}},	// Lv28
	{{651, 30}, {652, 20}},	// Lv29
	{{652, 30}, {2102, 30}},	// Lv30
	
	{{652, 50}, {2102, 45}},	// Lv31
	{{653, 25}, {2051, 40}},	// Lv32
	{{751, 10}, {2103, 30}},	// Lv33
	{{751, 30}, {2103, 40}},	// Lv34
	{{752, 10}, {2052, 30}},	// Lv35
	{{754, 20}, {2106, 20}},	// Lv36
	{{773, 20}, {2003, 20}},	// Lv37
	{{774, 40}, {2004, 30}},	// Lv38
	{{756, 30}, {2005, 30}},	// Lv39
	{{757, 30}, {2158, 30}},	// Lv40
	
	{{931, 10}, {2158, 35}},	// Lv41
	{{932, 10}, {2158, 40}},	// Lv42
	{{932, 20}, {2061, 35}},	// Lv43
	{{933, 10}, {2061, 30}},	// Lv44
	{{936, 50}, {2062, 45}},	// Lv45
	{{2062, 50}, {2054, 30}},	// Lv46
	{{933, 20}, {2132, 50}},	// Lv47
	{{934, 10}, {2132, 80}},	// Lv48
	{{905, 10}, {2063, 50}},	// Lv49
	{{2134, 30}, {905, 20}},	// Lv50
	
	{{735, 50}, {2108, 30}},	// Lv51
	{{934, 40}, {2064, 45}},	// Lv52
	{{934, 30}, {2064, 80}},	// Lv53
	{{736, 40}, {1001, 50}},	// Lv54
	{{777, 80}, {1001, 100}},	// Lv55
	{{935, 20}, {1002, 50}},	// Lv56
	{{935, 30}, {1002, 100}},	// Lv57
	{{1003, 45}, {1303, 40}},	// Lv58
	{{1002, 50}, {1303, 60}},	// Lv59
	{{937, 20}, {1003, 100}},	// Lv60
	
	{{1004, 20}, {1001, 200}},	// Lv61
	{{1305, 20}, {2065, 100}},	// Lv62
	{{1305, 50}, {1101, 45}},	// Lv63
	{{1301, 50}, {1102, 45}},	// Lv64
	{{1104, 40}, {2063, 80}},	// Lv65
	{{2301, 50}, {1105, 30}},	// Lv66
	{{2301, 80}, {1105, 50}},	// Lv67
	{{1106, 50}, {2305, 50}},	// Lv68
	{{1107, 45}, {2303, 50}},	// Lv69
	{{2302, 55}, {2201, 100}},	// Lv70
	
	{{2303, 80}, {2202, 100}},	// Lv71
	{{2303, 100}, {2202, 150}},	// Lv72
	{{2304, 55}, {2204, 100}},	// Lv73
	{{2305, 300}, {2204, 200}},	// Lv74
	{{1003, 3000}, {2204, 1500}},	// Lv75        --- DONE TILL HERE
	{{2302, 100}, {2401, 500}},	// Lv76
	{{2403, 100}, {2304, 200}},	// Lv77
	{{2403, 200}, {2301, 300}},	// Lv78
	{{2402, 500}, {2311, 100}},	// Lv79
	{{2313, 1500}, {2413, 500}},	// Lv80
	
	{{2312, 1000}, {2402, 1000}},	// Lv81
	{{2411, 250}, {2314, 50}},	// Lv82
	{{2315, 300}, {2414, 100}},	// Lv83
	{{2315, 400}, {2412, 200}},	// Lv84
	{{2493, 20}, {2492, 50}},	// Lv85
	{{2402, 4000}, {2314, 1000}},	// Lv86
	{{2402, 1500}, {2312, 2000}},	// Lv87
	{{2401, 5000}, {2311, 300}},	// Lv88
	{{2401, 6000}, {2312, 4000}},	// Lv89
	{{4138, 30}, {4137, 30}},	// Lv80
};

const DWORD THuntingRewardItem[HUNTING_MISSION_COUNT+1][2][4][2] =
{
	/*
	{ mission_type 1 = { warrior {vnum, count} | assassine {vnum, count} | sura {vnum, count} | shaman {vnum, count} } }
	{ mission_type 2 = { warrior {vnum, count} | assassine {vnum, count} | sura {vnum, count} | shaman {vnum, count} } }
	*/
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},										// None
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},									// Lv1 - Waffen
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv2 - Rustungen
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv3 - Schild
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv4 - Helm
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv5 - Armband
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv6 - Halskette
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv7 - Ohrringe
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv8 - Schuhe
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},			// Lv2 - Rustungen Lv9er
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv10
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv11
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv12
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv13
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv14
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv15
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv16
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv17
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv18
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv19
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv20
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv21
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv22
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv23
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv24
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv25
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv26
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv27
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv28
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv29
	{{{296, 1}, {296, 1}, {296, 1}, {7166, 1}}, {{3216, 1}, {1176, 1}, {296, 1}, {5116, 1}}},							// Lv30 - 30er Waffen
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv31
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv32
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv33
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv34
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv35
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv36
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv37
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv38
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv39
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv40
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv41
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv42
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv43
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv44
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv45
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv46
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv47
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv48
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv49
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv50
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv51
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv52
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv53
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv54
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv55
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv56
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv57
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv58
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv59
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv60
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv61
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv62
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv63
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv64
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv65
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv66
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv67
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv68
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv69
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv70
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv71
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv72
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv73
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv74
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv75
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv76
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv77
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv78
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv79
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv80
	
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv81
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv82
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv83
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv84
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv85
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv86
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv87
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv88
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv89
	{{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}},															// Lv90
};

const SHuntingRewardMoney THuntingRewardMoney[HUNTING_MONEY_TABLE_SIZE] =
{
	/*
	Note: The total of all Chances in Level = 100
	from_level, to_level {amount, chance} | {amount, chance} | {amount, chance} | {amount, chance} | {amount, chance}
	*/

	{ 1, 10,	{10000, 20000, 30000, 40000, 50000}},						// Lv1-10
	{11, 20,	{20000, 30000, 50000, 70000, 100000}},						// Lv11-20
	{21, 30,	{20000, 40000, 60000, 90000, 120000}},						// Lv21-30
	{31, 40,	{50000, 80000, 120000, 150000, 200000}},					// Lv31-40
	{41, 50,	{100000, 150000, 200000, 300000, 400000}},				// Lv41-50
	{51, 60,	{200000, 300000, 400000, 500000, 750000}},				// Lv51-60
	{61, 70,	{500000, 750000, 1000000, 1250000, 1500000}},			// Lv61-70
	{71, 80,	{2000000, 2500000, 3000000, 4000000, 5000000}},		// Lv71-80
	{81, 90,	{3000000, 4000000, 5000000, 10000000, 20000000}}, 	// Lv81-90
};

const SHuntingRewardEXP THuntingRewardEXP[HUNTING_EXP_TABLE_SIZE] =
{
	/*
	Note: percent exp from each level
	from_level, to_level {percent_min, percent_max}
	*/

	{ 1, 10, {10, 25}}, 	// Lv1-10
	{11, 20, {10, 20}},	// Lv11-20
	{21, 30, {10, 15}},	// Lv21-30
	{31, 40, {5, 15}},	// Lv31-40
	{41, 50, {5, 12}},	// Lv41-50
	{51, 60, {3, 10}},	// Lv51-60
	{61, 70, {3, 8}},		// Lv61-70
	{71, 80, {3, 5}}, 	// Lv71-80
	{81, 90, {2, 5}}, 	// Lv81-90
};

const DWORD THuntingRandomItem_01_20[6][2] =
{
	{11074, 20},	// Gruener Trank G
	{30522, 1},	// Violetter Trank G
	{51001, 10},	// Gruener Zauber
	{50513, 1},	// Gruene Staerke
	{27987, 1},	// Tapferkeitsumhang
	{25040, 2},	// Segensschriftrolle
};

const DWORD THuntingRandomItem_21_40[7][2] =
{
	{11074, 30},	// Segensschriftrolle
	{30522, 1},	// Magisches Metall
	{51001, 20},	// Affen-Ei
	{50513, 1},	// Spinnen-Ei
	{27987, 2},	// Fertigkeitsbuch
	{25040, 2},	// Fluchtlingsumhang
	{30006, 1},	// Fluchtlingsumhang
};

const DWORD THuntingRandomItem_41_60[8][2] =
{
	{11074, 40},	// Segensschriftrolle
	{30522, 2},	// Magisches Metall
	{50513, 1},	// Affen-Ei
	{27987, 3},	// Spinnen-Ei
	{25040, 3},	// Fertigkeitsbuch
	{50255, 1},	// Fluchtlingsumhang
	{25041, 1},	// Lucys Ring
	{30047, 1},	// Symbol d. weisen Kaisers
};

const DWORD THuntingRandomItem_61_80[7][2] =
{
	{11074, 70},	// Segensschriftrolle
	{30522, 1},	// Magisches Metall
	{50513, 1},	// Affen-Ei
	{27987, 5},	// Spinnen-Ei
	{25040, 10},	// Fertigkeitsbuch
	{50255, 2},	// Fluchtlingsumhang
	{25041, 2},	// Lucys Ring
};

const DWORD THuntingRandomItem_81_90[12][2] =
{
	{11074, 150},	// Segensschriftrolle
	{50255, 2},	// Magisches Metall
	{51001, 50},	// Affen-Ei
	{49990, 1},	// Spinnen-Ei
	{25041, 1},	// Fertigkeitsbuch
	{27987, 5},	// Fluchtlingsumhang
	{39017, 2},	// Lucys Ring
	{39018, 2},	// Lucys Ring
	{39019, 2},	// Lucys Ring
	{39020, 2},	// Lucys Ring
	{49990, 2},	// Lucys Ring
	{50255, 3},	// Lucys Ring
};
#endif


//martysama0134's 2022
