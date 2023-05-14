#include "stdafx.h"

enum
{
	COMMAND_TYPE_DELIVER,
	COMMAND_TYPE_RESET_COOLDOWN,
};

class CBiolog : public singleton<CBiolog>
{
	public:
		CBiolog();
		virtual ~CBiolog();


        void BootBiologTable();
		bool GetBiologMap(uint8_t id, TBiologTable** pBiologInformation);
		bool DeliverItem(LPCHARACTER pkChar, int16_t option = -1, bool morePercentage = false, bool resetDeliver = false);

		void AddBonusToChar(LPCHARACTER pkChar, int16_t option);
		std::tuple<uint8_t, uint32_t> GetNextBiologInfo(DWORD nextLevel);
		
		void SendBiologData(LPCHARACTER pkChar);

		void ResetBiolog(LPCHARACTER pkChar);

		void OnConnect(LPCHARACTER pkChar);
    private:
	    typedef std::unordered_map<DWORD, TBiologTable> TBiologCacheMap;
	    TBiologCacheMap	m_map_BiologTable;


		uint8_t biologMaxLimit = 1;

};
