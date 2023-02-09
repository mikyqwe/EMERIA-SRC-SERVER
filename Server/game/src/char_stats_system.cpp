#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "battle.h"
#include "pvp.h"
#include "skill.h"
#include "start_position.h"
#include "profiler.h"
#include "cmd.h"
#include "dungeon.h"
#include "log.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "db.h"
#include "vector.h"
#include "marriage.h"
#include "arena.h"
#include "regen.h"
#include "monarch.h"
#include "exchange.h"
#include "shop_manager.h"
#include "castle.h"
#include "dev_log.h"
#include "ani.h"
#include "BattleArena.h"
#include "packet.h"
#include "party.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "questmanager.h"
#include "questlua.h"
#include "threeway_war.h"
#include "BlueDragon.h"
#include "DragonLair.h"



void CHARACTER::LoadStats(TSpecialStats * sdata) {

	memset(&special_stats, 0, sizeof(special_stats));
	for (int i = 0; i < SPECIALSTATS_MAX; i++) {
		special_stats[i] = sdata->s_stats[i];
		if(special_stats[i] > 0){
			ApplySpecialStatBuff(i);
		}
		//sys_log(0, "Nuova Statisca del player Caricata: [%d] ---> %u", i, special_stats[i]);
	}

	special_next_book_time = sdata->next_book_time;
	special_next_book_time_skip = sdata->skip_time_book;
	
}

bool CHARACTER::CanReadSpecialStatBooks() {
	return special_next_book_time_skip || special_next_book_time <= get_global_time();
}

bool CHARACTER::CanReadSpecialStatBookSkip() {
	return !special_next_book_time_skip;
}

void CHARACTER::SetSpecialStatsSkillBookTimeSkip() {
	special_next_book_time_skip = 1;
	SaveSpecialStats();
}

void CHARACTER::SendSpecialStatsEffect() {
	struct packet_point_change pack;

	pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
	pack.dwVID = GetVID();
	pack.type = POINT_SPECIAL_STAT_EFFECT;
	pack.value = 1;
	pack.amount = 1;
	PacketAround(&pack, sizeof(pack));
}

void CHARACTER::SetSpecialStatsNextTimeReadBook() {
	special_next_book_time = get_global_time() + 24*60*60;
	special_next_book_time_skip = 0;
	SaveSpecialStats();
}

bool CHARACTER::CanUpdateSpecialStat(BYTE s_stat) {
	return special_stats[s_stat] < SPECIALSTAT_MAX_LEVEL;
}

BYTE CHARACTER::GetSpecialStats(BYTE s_stat){
	return special_stats[s_stat];
}

void CHARACTER::UpdateSpecialStat(BYTE s_stat) {
	special_stats[s_stat] += 1;
	SendSpecialStatsEffect();
	SpecialStatsPacket();
	ApplySpecialStatBuff(s_stat);
	SaveSpecialStats();
	
}

void CHARACTER::ChatSpecialStats() {
	for (int i = 0; i < SPECIALSTATS_MAX; i++) {
		ChatPacket(CHAT_TYPE_INFO, "NuoveStatiche Player: [%d] --> %u", i, special_stats[i]);
	}
}


void  CHARACTER::SpecialStatsPacket() {
	for (int i = 0; i < SPECIALSTATS_MAX; i++) {
		if (FindAffect(SPECIALSTAT_AFFECT_START + i))
			RemoveAffect(SPECIALSTAT_AFFECT_START + i);
	}
	ChatPacket(CHAT_TYPE_COMMAND, "RefreshSStat %u %u %u %u %u %u", this->special_stats[0], this->special_stats[1], this->special_stats[2], this->special_stats[3], this->special_stats[4], this->special_stats[5]);
}

void  CHARACTER::SaveSpecialStats() {
	DBManager::instance().DirectQuery("UPDATE player_specialstats SET frt = %u, agl=%u, crm=%u, tnc=%u, avd=%u, tlt=%u, next_book_readtime=%u, skip_time_book=%u WHERE pid = %lu ", this->special_stats[0], this->special_stats[1], this->special_stats[2], this->special_stats[3], this->special_stats[4], this->special_stats[5], special_next_book_time, special_next_book_time_skip,this->GetPlayerID());
}

void CHARACTER::ApplySpecialStatBuff(BYTE s_stat) {
	ClearSpecialStatAffect(s_stat);

	switch (s_stat) {

		case SPECIALSTAT2:
			ApplySpecialStatAffect(s_stat, POINT_ATT_SPEED, 1);
			ApplySpecialStatAffect(s_stat, POINT_MOV_SPEED, 2);
			break;
		case SPECIALSTAT3:
			ApplySpecialStatAffect(s_stat, POINT_ATTBONUS_MONSTER, 1);
			ApplySpecialStatAffect(s_stat, POINT_MAX_HP, 100);
			break;

		case SPECIALSTAT4:
			ApplySpecialStatAffect(s_stat, POINT_CRITICAL_PCT, 1);
			ApplySpecialStatAffect(s_stat, POINT_ATT_GRADE, 10);
			break;

		case SPECIALSTAT5:
			ApplySpecialStatAffect(s_stat, POINT_ITEM_DROP_BONUS, 1);
			ApplySpecialStatAffect(s_stat, POINT_GOLD_DOUBLE_BONUS, 2);
			break;

		case SPECIALSTAT6:
			ApplySpecialStatAffect(s_stat, POINT_ATTBONUS_WARRIOR, 1);
			ApplySpecialStatAffect(s_stat, POINT_ATTBONUS_ASSASSIN, 1);
			ApplySpecialStatAffect(s_stat, POINT_ATTBONUS_SURA, 1);
			ApplySpecialStatAffect(s_stat, POINT_ATTBONUS_SHAMAN, 1);
			break;
	}
}

void CHARACTER::ApplySpecialStatAffect(BYTE s_stat, BYTE s_type, long s_value) {	
	AddAffect(SPECIALSTAT_AFFECT_START + s_stat, s_type, s_value * GetSpecialStats(s_stat), 0, 60 * 60 * 24 * 365, 0, false);
}

void CHARACTER::ClearSpecialStatAffect(BYTE s_stat) {
	RemoveAffect(SPECIALSTAT_AFFECT_START + s_stat);
}
