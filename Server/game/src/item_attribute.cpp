#include "stdafx.h"
#include "constants.h"
#include "log.h"
#include "item.h"
#include "char.h"
#include "desc.h"
#include "item_manager.h"
#ifdef ENABLE_NEWSTUFF
#include "config.h"
#endif

#ifndef ENABLE_SWITCHBOT
const int MAX_NORM_ATTR_NUM = ITEM_MANAGER::MAX_NORM_ATTR_NUM;
const int MAX_RARE_ATTR_NUM = ITEM_MANAGER::MAX_RARE_ATTR_NUM;
#endif

int CItem::GetAttributeSetIndex()
{
	if (GetType() == ITEM_WEAPON)
	{
		if (GetSubType() == WEAPON_ARROW)
			return -1;

		return ATTRIBUTE_SET_WEAPON;
	}

	if (GetType() == ITEM_ARMOR)
	{
		switch (GetSubType())
		{
			case ARMOR_BODY:
				return ATTRIBUTE_SET_BODY;

			case ARMOR_WRIST:
				return ATTRIBUTE_SET_WRIST;

			case ARMOR_FOOTS:
				return ATTRIBUTE_SET_FOOTS;

			case ARMOR_NECK:
				return ATTRIBUTE_SET_NECK;

			case ARMOR_HEAD:
				return ATTRIBUTE_SET_HEAD;

			case ARMOR_SHIELD:
				return ATTRIBUTE_SET_SHIELD;

			case ARMOR_EAR:
				return ATTRIBUTE_SET_EAR;
			
			#ifdef ENABLE_NEW_TALISMAN_GF
			case ARMOR_TALISMAN:
				#ifdef ENABLE_NEW_TALISMAN_SLOTS_NEW
				if(GetVnum() >= 9600 && GetVnum() <= 9800)
					return ATTRIBUTE_SET_TALISMAN;
				else if(GetVnum() >= 9830 && GetVnum() <= 10030)
					return ATTRIBUTE_SET_TALISMAN_2;
				else if(GetVnum() >= 10060 && GetVnum() <= 10260)
					return ATTRIBUTE_SET_TALISMAN_3;
				else if(GetVnum() >= 10290 && GetVnum() <= 10490)
					return ATTRIBUTE_SET_TALISMAN_4;
				else if(GetVnum() >= 10520 && GetVnum() <= 10720)
					return ATTRIBUTE_SET_TALISMAN_5;
				else if(GetVnum() >= 10750 && GetVnum() <= 10950)
					return ATTRIBUTE_SET_TALISMAN_6;
				#else
				return ATTRIBUTE_SET_TALISMAN;
				#endif
			#endif
			#if defined(ENABLE_NEW_TALISMAN_SLOTS) && !defined(ENABLE_NEW_TALISMAN_SLOTS_NEW)
			case ARMOR_TALISMAN_2:
				return ATTRIBUTE_SET_TALISMAN_2;
			case ARMOR_TALISMAN_3:
				return ATTRIBUTE_SET_TALISMAN_3;
			case ARMOR_TALISMAN_4:
				return ATTRIBUTE_SET_TALISMAN_4;
			case ARMOR_TALISMAN_5:
				return ATTRIBUTE_SET_TALISMAN_5;
			case ARMOR_TALISMAN_6:
				return ATTRIBUTE_SET_TALISMAN_6;
			#endif
		}
	}
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
			case COSTUME_BODY: // 코스츔 갑옷은 일반 갑옷과 동일한 Attribute Set을 이용하여 랜덤속성 붙음 (ARMOR_BODY == COSTUME_BODY)
				return ATTRIBUTE_SET_BODY;

			case COSTUME_HAIR: // 코스츔 헤어는 일반 투구 아이템과 동일한 Attribute Set을 이용하여 랜덤속성 붙음 (ARMOR_HEAD == COSTUME_HAIR)
				return ATTRIBUTE_SET_HEAD;

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			case COSTUME_MOUNT:
				break;
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			case COSTUME_WEAPON:
				return ATTRIBUTE_SET_WEAPON;
#endif

		}
	}

	return -1;
}

bool CItem::HasAttr(BYTE bApply)
{
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		if (m_pProto->aApplies[i].bType == bApply)
			return true;

	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
		if (GetAttributeType(i) == bApply)
			return true;

	return false;
}

bool CItem::HasRareAttr(BYTE bApply)
{
	for (int i = 0; i < MAX_RARE_ATTR_NUM; ++i)
		if (GetAttributeType(i + 5) == bApply)
			return true;

	return false;
}

void CItem::AddAttribute(BYTE bApply, short sValue)
{
	if (HasAttr(bApply))
		return;

	int i = GetAttributeCount();

	if (i >= MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		if (sValue)
			SetAttribute(i, bApply, sValue);
	}
}

void CItem::AddAttr(BYTE bApply, BYTE bLevel)
{
	if (HasAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = GetAttributeCount();

	if (i == MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		const TItemAttrTable & r = g_map_itemAttr[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetAttribute(i, bApply, lVal);
	}
}

#ifdef ENABLE_USE_DIFFERENT_TABLE_FOR_COSTUME_ATTRIBUTE
void CItem::AddCostumeAttr(BYTE bApply, BYTE bLevel)
{	
	if (HasAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = GetAttributeCount();

	if (i == MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		const TItemAttrTable & r = g_map_itemCostumeAttr[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetAttribute(i, bApply, lVal);
	}
	
}

void CItem::PutCostumeAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	// 붙일 수 있는 속성 배열을 구축
	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemCostumeAttr[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	// 구축된 배열로 확률 계산을 통해 붙일 속성 선정
	unsigned int prob = number(1, total);
	int attr_idx = APPLY_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable & r = g_map_itemCostumeAttr[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable & r = g_map_itemCostumeAttr[attr_idx];

	// 종류별 속성 레벨 최대값 제한
	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];
	AddCostumeAttr(attr_idx, bLevel);
}



void CItem::PutCostumeAttribute(const int * aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}
	
	PutCostumeAttributeWithLevel(i + 1);
}

#endif

void CItem::PutAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	// 붙일 수 있는 속성 배열을 구축
	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemAttr[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	// 구축된 배열로 확률 계산을 통해 붙일 속성 선정
	unsigned int prob = number(1, total);
	int attr_idx = APPLY_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable & r = g_map_itemAttr[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable & r = g_map_itemAttr[attr_idx];

	// 종류별 속성 레벨 최대값 제한
	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddAttr(attr_idx, bLevel);
}

void CItem::PutAttribute(const int * aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutAttributeWithLevel(i + 1);
}

void CItem::ChangeAttribute(const int* aiChangeProb)
{
	int iAttributeCount = GetAttributeCount();

	ClearAttribute();

	if (iAttributeCount == 0)
		return;

	TItemTable const * pProto = GetProto();

	if (pProto && pProto->sAddonType)
	{
		ApplyAddon(pProto->sAddonType);
	}

	static const int tmpChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		0, 10, 40, 35, 15,
	};

	for (int i = GetAttributeCount(); i < iAttributeCount; ++i)
	{
		if (aiChangeProb == NULL)
		{
			PutAttribute(tmpChangeProb);
		}
		else
		{
			PutAttribute(aiChangeProb);
		}
	}
}


#ifdef ENABLE_USE_DIFFERENT_TABLE_FOR_COSTUME_ATTRIBUTE
void CItem::ChangeCostumeAttribute(const int* aiChangeProb)
{
	int iAttributeCount = GetAttributeCount();

	ClearAttribute();

	if (iAttributeCount == 0)
		return;

	TItemTable const * pProto = GetProto();

	if (pProto && pProto->sAddonType)
	{
		ApplyAddon(pProto->sAddonType);
	}

	static const int tmpChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		0, 10, 40, 35, 15,
	};

	for (int i = GetAttributeCount(); i < iAttributeCount; ++i)
	{
		if (aiChangeProb == NULL)
		{
			PutCostumeAttribute(tmpChangeProb);
		}
		else
		{
			PutCostumeAttribute(aiChangeProb);
		}
	}
}
#endif

void CItem::AddAttribute()
{
	static const int aiItemAddAttributePercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		40, 50, 10, 0, 0
	};

	if (GetAttributeCount() < MAX_NORM_ATTR_NUM)
		PutAttribute(aiItemAddAttributePercent);
}

void CItem::ClearAttribute()
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
#ifndef __FROZENBONUS_SYSTEM__
		m_aAttr[i].bType = 0;
		m_aAttr[i].sValue = 0;
#else
		if (!m_aAttr[i].isFrozen) {
			m_aAttr[i].bType = 0;
			m_aAttr[i].sValue = 0;
		}
#endif
	}
}

#ifdef ENABLE_DRAGON_SOUL_CHANGE_BONUS_WORLDARD
void CItem::ClearAttributeDragonSoul()
{

	for (int i = 3; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
	{
		m_aAttr[i].bType = 0;
		m_aAttr[i].sValue = 0;
	}
}
#endif

int CItem::GetAttributeCount()
{
	int i;

	for (i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == 0)
			break;
	}

	return i;
}

int CItem::FindAttribute(BYTE bType)
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == bType)
			return i;
	}

	return -1;
}

bool CItem::RemoveAttributeAt(int index)
{
	if (GetAttributeCount() <= index)
		return false;


	for (int i = index; i < MAX_NORM_ATTR_NUM - 1; ++i)
	{
		SetAttribute(i, GetAttributeType(i + 1), GetAttributeValue(i + 1));
	}

	SetAttribute(MAX_NORM_ATTR_NUM - 1, APPLY_NONE, 0);
	return true;
}

bool CItem::RemoveAttributeType(BYTE bType)
{
    int index = FindAttribute(bType);

#ifdef __FROZENBONUS_SYSTEM__
	if (index != -1 && m_aAttr[index].isFrozen)
		return false;
#endif // __FROZENBONUS_SYSTEM__
    return index != -1 && RemoveAttributeAt(index);
}

void CItem::SetAttributes(const TPlayerItemAttribute* c_pAttribute)
{
	thecore_memcpy(m_aAttr, c_pAttribute, sizeof(m_aAttr));
	Save();
}

void CItem::SetAttribute(int i, BYTE bType, short sValue)
{
	assert(i < MAX_NORM_ATTR_NUM);

#ifdef __FROZENBONUS_SYSTEM__
	if (m_aAttr[i].isFrozen)
		return;
#endif

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		const char * pszIP = NULL;

		if (GetOwner() && GetOwner()->GetDesc())
			pszIP = GetOwner()->GetDesc()->GetHostName();

		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(i, bType, sValue, GetID(), "SET_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum()));
	}
}

void CItem::SetForceAttribute(int i, BYTE bType, short sValue)
{
	assert(i < ITEM_ATTRIBUTE_MAX_NUM);

	if (i >= ITEM_ATTRIBUTE_MAX_NUM)
	{
		sys_err("attr index overflow! index %d type %u value %d  (owner %s)", i, bType, sValue, GetOwner() ? GetOwner()->GetName() : "NO_OWNER");
		return;
	}

#ifdef __FROZENBONUS_SYSTEM__
	if (m_aAttr[i].isFrozen)
		return;
#endif

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		const char * pszIP = NULL;

		if (GetOwner() && GetOwner()->GetDesc())
			pszIP = GetOwner()->GetDesc()->GetHostName();

		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(i, bType, sValue, GetID(), "SET_FORCE_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum()));
	}
}

void CItem::SetForceAttributeAcce(int i, BYTE bType, short sValue)
{
	assert(i < ITEM_ATTRIBUTE_MAX_NUM);
	
	if (i >= ITEM_ATTRIBUTE_MAX_NUM)
	{
		sys_err("attr index overflow! index %d type %u value %d  (owner %s)", i, bType, sValue, GetOwner() ? GetOwner()->GetName() : "NO_OWNER");
		return;
	}
	
	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		const char * pszIP = NULL;

		if (GetOwner() && GetOwner()->GetDesc())
			pszIP = GetOwner()->GetDesc()->GetHostName();

		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(i, bType, sValue, GetID(), "SET_FORCE_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum()));
	}
}


void CItem::CopyAttributeTo(LPITEM pItem)
{
	pItem->SetAttributes(m_aAttr);
}

int CItem::GetRareAttrCount()
{
	int ret = 0;

	for (DWORD dwIdx = ITEM_ATTRIBUTE_RARE_START; dwIdx < ITEM_ATTRIBUTE_RARE_END; dwIdx++)
	{
		if (m_aAttr[dwIdx].bType != 0)
			ret++;
	}

	return ret;
}

bool CItem::ChangeRareAttribute()
{
	if (GetRareAttrCount() == 0)
		return false;

	int cnt = GetRareAttrCount();

	for (int i = 0; i < cnt; ++i)
	{
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].bType = 0;
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].sValue = 0;
	}

	if (GetOwner() && GetOwner()->GetDesc())
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(GetOwner(), this, "SET_RARE_CHANGE", ""))
	else
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(0, 0, 0, GetID(), "SET_RARE_CHANGE", "", "", GetOriginalVnum()))

	for (int i = 0; i < cnt; ++i)
	{
		AddRareAttribute();
	}

	return true;
}

bool CItem::AddRareAttribute()
{
	int count = GetRareAttrCount();

	if (count >= ITEM_ATTRIBUTE_RARE_NUM)
		return false;
	int pos = count + ITEM_ATTRIBUTE_RARE_START;
	TPlayerItemAttribute & attr = m_aAttr[pos];

	int nAttrSet = GetAttributeSetIndex();
	std::vector<int> avail;

	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[i];

		if (r.dwApplyIndex != 0 && r.bMaxLevelBySet[nAttrSet] > 0 && HasRareAttr(i) != true)
		{
			avail.push_back(i);
		}
	}

	const TItemAttrTable& r = g_map_itemRare[avail[number(0, avail.size() - 1)]];
	int nAttrLevel = (rand() % 4) + 1;

	if (nAttrLevel > r.bMaxLevelBySet[nAttrSet])
		nAttrLevel = r.bMaxLevelBySet[nAttrSet];

	attr.bType = r.dwApplyIndex;
	attr.sValue = r.lValues[nAttrLevel - 1];

	UpdatePacket();

	Save();

	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(pos, attr.bType, attr.sValue, GetID(), "SET_RARE", "", pszIP ? pszIP : "", GetOriginalVnum()));
	return true;
}

void CItem::AddRareAttribute2(const int * aiAttrPercentTable)
{
	static const int aiItemAddAttributePercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		40, 50, 10, 0, 0
	};
	if (aiAttrPercentTable == NULL)
		aiAttrPercentTable = aiItemAddAttributePercent;

	if (GetRareAttrCount() < MAX_RARE_ATTR_NUM)
		PutRareAttribute(aiAttrPercentTable);
}

void CItem::PutRareAttribute(const int * aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutRareAttributeWithLevel(i + 1);
}

void CItem::PutRareAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	// 붙일 수 있는 속성 배열을 구축
	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasRareAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	// 구축된 배열로 확률 계산을 통해 붙일 속성 선정
	unsigned int prob = number(1, total);
	int attr_idx = APPLY_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item rare attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable & r = g_map_itemRare[attr_idx];

	// 종류별 속성 레벨 최대값 제한
	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddRareAttr(attr_idx, bLevel);
}

void CItem::AddRareAttr(BYTE bApply, BYTE bLevel)
{
	if (HasRareAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = ITEM_ATTRIBUTE_RARE_START + GetRareAttrCount();

	if (i == ITEM_ATTRIBUTE_RARE_END)
		sys_err("item rare attribute overflow!");
	else
	{
		const TItemAttrTable & r = g_map_itemRare[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetForceAttribute(i, bApply, lVal);
	}
}

#ifdef __FROZENBONUS_SYSTEM__

bool CItem::CanFrozenBonus(int index) {
	TItemTable const * pProto = GetProto();

	if (GetAttributeCount() <= index)
		return false;

	if (pProto && pProto->sAddonType && index < 2) {
		return HasAttr(APPLY_NORMAL_HIT_DAMAGE_BONUS) && HasAttr(APPLY_SKILL_DAMAGE_BONUS);
	}
	return true;
}

BYTE CItem::GetFrozenAttribute() {
	BYTE count = 0;

	for (BYTE i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++) {
		if (m_aAttr[i].isFrozen)
			count++;
	}
	return count;
}

bool CItem::IsFrozenBonus(int index) {
	return m_aAttr[index].isFrozen;
}

bool CItem::SetFrozenBonus(int index, bool isFrozen) {

	if (!CanFrozenBonus(index) && isFrozen)
		return false;
	
	if (index >= ITEM_ATTRIBUTE_MAX_NUM)
	{
		sys_err("attr index overflow! index %d  (owner %s)", index, GetOwner() ? GetOwner()->GetName() : "NO_OWNER");
		return false;
	}

	TItemTable const * pProto = GetProto();

	m_aAttr[index].isFrozen = isFrozen;

	if (pProto && pProto->sAddonType && index < 2) {
		m_aAttr[0].isFrozen = isFrozen;
		m_aAttr[1].isFrozen = isFrozen;
	}

	UpdatePacket();
	Save();

	return true;

}

bool CItem::SetForceFrozenBonus(int index, bool isFrozen) {
	m_aAttr[index].isFrozen = isFrozen;
	return true;
}

#endif

