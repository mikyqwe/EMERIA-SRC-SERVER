#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"�ż���",
	"õ����",
	"���뱹",
};

//	LC_TEXT("�ż���")
//	LC_TEXT("õ����")
//	LC_TEXT("���뱹")

long g_start_map[4] =
{
	0, // reserved
	1, // �ż���
	21, // õ����
	41 // ���뱹
};

DWORD g_start_position[4][2] =
{
	{ 0, 0 }, // reserved
	{ 1021200, 274700 }, // Shinsoo
	{ 1021700, 275700 }, // õ����
	{ 963600, 269500 } // ���뱹
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


