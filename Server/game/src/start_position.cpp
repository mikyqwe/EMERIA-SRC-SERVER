#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"신수국",
	"천조국",
	"진노국",
};

//	LC_TEXT("신수국")
//	LC_TEXT("천조국")
//	LC_TEXT("진노국")

long g_start_map[4] =
{
	0, // reserved
	1, // 신수국
	21, // 천조국
	41 // 진노국
};

DWORD g_start_position[4][2] =
{
	{ 0, 0 }, // reserved
	{ 1021200, 274700 }, // Shinsoo
	{ 1021700, 275700 }, // 천조국
	{ 963600, 269500 } // 진노국
};


DWORD arena_return_position[4][2] =
{
	{ 0, 0 },
	{ 805700, 15500 },// red empire
	{ 805700, 15500 },// yellow empire
	{ 805700, 15500 } // blue empire
};


DWORD g_create_position[4][2] =
{
	{ 0, 0 }, // reserved,
	{ 1021200, 274700 },
	{ 1021700, 275700 },
	{ 963600, 269500 },
};


DWORD g_create_position_canada[4][2] =
{
	{		0,		0 },
	{ 1021200, 274700 },
	{ 1021700, 275700 },
	{ 963600, 269500 },
};


