#ifdef __BATTLE_PASS__
void CHARACTER::DoMission(int index, long long count)
{
	if (missions_bp.empty())
		return;
	
	if (v_counts[index].status > 0)
		return;
	
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	int month = vKey.tm_mon;
	
	if (month != iMonthBattlePass)
		return;
	
	FILE 	*fileID;
	char file_name[256+1];

	snprintf(file_name, sizeof(file_name), "%s/battlepass_players/%s.txt", LocaleService_GetBasePath().c_str(),GetName());
	fileID = fopen(file_name, "w");
	
	if (NULL == fileID)
		return;
	
	v_counts[index].count = v_counts[index].count + count;
	
	if (v_counts[index].count > missions_bp[index].count)
		v_counts[index].count = missions_bp[index].count;
	
	if (missions_bp[index].count == v_counts[index].count)
	{
		v_counts[index].status = v_counts[index].status + 1;
		ChatPacket(CHAT_TYPE_INFO, "Ai terminat o misiune din Battle-Pass, Felicitari!");
		AutoGiveItem(rewards_bp[index].vnum1,rewards_bp[index].count1);
		AutoGiveItem(rewards_bp[index].vnum2,rewards_bp[index].count2);
		AutoGiveItem(rewards_bp[index].vnum3,rewards_bp[index].count3);
	}

	for (int i=0; i<v_counts.size(); ++i)
	{
		fprintf(fileID,"MISSION	%d	%d\n", v_counts[i].count, v_counts[i].status);
	}

	fclose(fileID);
	
	SendInfosBattlePass(index);
}

void CHARACTER::Load_BattlePass()
{
	FILE 	*fp;
	char	one_line[256];
	int		value1, value2;
	const char	*delim = " \t\r\n";
	char	*v, *token_string;
	char file_name[256+1];

	Struct_BattlePass save_counts = {0, 0};

	snprintf(file_name, sizeof(file_name), "%s/battlepass_players/%s.txt", LocaleService_GetBasePath().c_str(),GetName());
	fp = fopen(file_name, "r");

	if (NULL == fp)
		return;

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = 0;

		if (one_line[0] == '#')
			continue;
		
		token_string = strtok(one_line, delim);
		if (NULL == token_string)
			continue;
		
		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		TOKEN("MISSION")
		{
			save_counts.count = value1;
			save_counts.status = value2;
			v_counts.push_back(save_counts);
		}
	}
	fclose(fp);
	
	ChatPacket(CHAT_TYPE_COMMAND, "status_battlepass %d", 1);
	ChatPacket(CHAT_TYPE_COMMAND, "battlepass_status %d", 1);
}

void CHARACTER::ExternBattlePass()
{
	FILE 	*fp;
	char	one_line[256];
	int		value1, value2, value3, value4, value5, value6;
	const char	*delimit = " \t\r\n";
	char	*v, *token_string;
	char value7[4096];
	char file_name[256+1];

	Infos_BattlePass bp_infos = {0,0,0,0,0,0, 0};
	Infos_MissionsBP bp_missions = {0,0,0};
	Infos_FinalBP bp_final_reward = {0,0,0,0};
	
	snprintf(file_name, sizeof(file_name), "%s/battlepass_config.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");


	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = value3 = value4 = value5 = value6 = 0;

		if (one_line[0] == '#')
			continue;
		
		token_string = strtok(one_line, delimit);
		if (NULL == token_string)
			continue;

		if ((v = strtok(NULL, delimit)))
			str_to_number(value1, v);
		
		if ((v = strtok(NULL, delimit)))
			str_to_number(value2, v);

		if ((v = strtok(NULL, delimit)))
			str_to_number(value3, v);
	
		if ((v = strtok(NULL, delimit)))
			str_to_number(value4, v);
		
		if ((v = strtok(NULL, delimit)))
			str_to_number(value5, v);
		
		if ((v = strtok(NULL, delimit)))
			str_to_number(value6, v);
		
		if ((v = strtok(NULL, delimit)))
			strlcpy(bp_infos.name, v, sizeof(bp_infos.name) - 1);
		
		// New Image
		if ((v = strtok(NULL, delimit)))
			strlcpy(bp_infos.image, v, sizeof(bp_infos.image) - 1);


		TOKEN("REWARDS_AND_NAME")
		{
			bp_infos.vnum1 = value1;
			bp_infos.count1  = value2;
			bp_infos.vnum2 = value3;
			bp_infos.count2  = value4;
			bp_infos.vnum3 = value5;
			bp_infos.count3  = value6;
			rewards_bp.push_back(bp_infos);
		}
		else TOKEN("DO_MISSION")
		{
			bp_missions.type = value1;
			bp_missions.vnum  = value2;
			bp_missions.count = value3;
			missions_bp.push_back(bp_missions);
		}
		else TOKEN("FINAL_REWARD")
		{
			bp_final_reward.f_vnum1 = value1;
			bp_final_reward.f_count1  = value2;
			bp_final_reward.f_vnum2 = value3;
			bp_final_reward.f_count2  = value4;
			bp_final_reward.f_vnum3 = value5;
			bp_final_reward.f_count3  = value6;
			final_rewards.push_back(bp_final_reward);
		}
		else TOKEN("MONTH_SEASON")
		{
			iMonthBattlePass = value1;
		}

	}
	fclose(fp);
}

void CHARACTER::SendInfosBattlePass(int index)
{
	if (v_counts.empty())
		return;
	
	if (missions_bp.empty())
		return;

	ChatPacket(CHAT_TYPE_COMMAND, "info_missions_bp %d %d %d %s %s", index, v_counts[index].count, v_counts[index].status, rewards_bp[index].name, rewards_bp[index].image);
}

void CHARACTER::FinalRewardBattlePass()
{
	if (v_counts.empty())
		return;
	
	if (missions_bp.empty())
		return;

	if (final_rewards.empty())
		return;
	
	if (v_counts[0].status == 2)
		return;
	
	FILE 	*fileID;
	char file_name[256+1];

	snprintf(file_name, sizeof(file_name), "%s/battlepass_players/%s.txt", LocaleService_GetBasePath().c_str(),GetName());
	fileID = fopen(file_name, "w");
	
	if (NULL == fileID)
		return;

	// Set Final Reward
	v_counts[0].status = v_counts[0].status + 1;

	for (int i=0; i<v_counts.size(); ++i)
	{
		fprintf(fileID,"MISSION	%d	%d\n", v_counts[i].count, v_counts[i].status);
	}
	
	fclose(fileID);
	
	// Get Final Reward
	AutoGiveItem(final_rewards[0].f_vnum1,final_rewards[0].f_count1);
	AutoGiveItem(final_rewards[0].f_vnum2,final_rewards[0].f_count2);
	AutoGiveItem(final_rewards[0].f_vnum3,final_rewards[0].f_count3);
}
#endif
