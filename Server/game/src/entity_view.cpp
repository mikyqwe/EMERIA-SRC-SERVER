#include "stdafx.h"

#include "utils.h"
#include "char.h"
#include "sectree_manager.h"
#include "config.h"

void CEntity::ViewCleanup()
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = it->first;
		++it;

		entity->ViewRemove(this, false);
	}

	m_map_view.clear();
}

void CEntity::ViewReencode()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		if (!m_bIsObserver)
			EncodeInsertPacket(entity);

		if (!entity->m_bIsObserver)
			entity->EncodeInsertPacket(this);
	}

}

void CEntity::ViewInsert(LPENTITY entity, bool recursive)
{
	if (this == entity)
		return;

	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (m_map_view.end() != it)
	{
		it->second = m_iViewAge;
		return;
	}

	m_map_view.insert(ENTITY_MAP::value_type(entity, m_iViewAge));

	if (!entity->m_bIsObserver)
		entity->EncodeInsertPacket(this);

	if (recursive)
		entity->ViewInsert(this, false);
}

void CEntity::ViewRemove(LPENTITY entity, bool recursive)
{
	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (it == m_map_view.end())
		return;

	m_map_view.erase(it);

	if (!entity->m_bIsObserver)
		entity->EncodeRemovePacket(this);

	if (recursive)
		entity->ViewRemove(this, false);
}

class CFuncViewInsert
{
    private:
        int dwViewRange;

    public:
        LPENTITY m_me;

        CFuncViewInsert(LPENTITY ent) :
            dwViewRange(VIEW_RANGE + VIEW_BONUS_RANGE),
            m_me(ent)
        {
        }

        void operator () (LPENTITY ent)
        {
            if (!ent->IsType(ENTITY_OBJECT))
                if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > dwViewRange)
                    return;


            if (m_me->IsType(ENTITY_CHARACTER)) {
                LPCHARACTER ch_me = (LPCHARACTER)m_me;
                if (ch_me->IsPC()) {
                    m_me->ViewInsert(ent);  //the players see everything..
                } else if (ch_me->IsNPC() && ent->IsType(ENTITY_CHARACTER)) {
                    LPCHARACTER ch_ent = (LPCHARACTER)ent;
                    if (ch_ent->IsPC()) {
                        m_me->ViewInsert(ent); //the npcs see the players...
                    }
                    else if (ch_ent->IsNPC()) {
                        /* JOTUN/OCHAO/HYDRA CONTENT, WE DONT NEED THIS RIGHT NOW BUT REMEMBER REMEMBER THE 6th OF NOVEMBER
                        if (IS_SET(ch_me->GetAIFlag(), AIFLAG_HEALER)) {
                            m_me->ViewInsert(ent); //the npc-healers see other npcs (ochao fix)
                        } else {
                            switch (ch_ent->GetRaceNum()) {
                                case 20434: {
                                    m_me->ViewInsert(ent); //the npcs can be seen by other npcs (hydra sail fix)
                                } break;
                            }
                        }
                        */
                    }
                }
            } else {
                m_me->ViewInsert(ent);
            }


            if (ent->IsType(ENTITY_CHARACTER) && m_me->IsType(ENTITY_CHARACTER))
            {
                LPCHARACTER chMe = (LPCHARACTER) m_me;
                LPCHARACTER chEnt = (LPCHARACTER) ent;

                if (chMe->IsPC() && !chEnt->IsPC() && !chEnt->IsWarp() && !chEnt->IsGoto())
                    chEnt->StartStateMachine();
            }
        }
};

void CEntity::UpdateSectree()
{
	if (!GetSectree())
	{
		if (IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER tch = (LPCHARACTER) this;
			sys_err("null sectree name: %s %d %d",  tch->GetName(), GetX(), GetY());
		}

		return;
	}

	++m_iViewAge;

	CFuncViewInsert f(this);
	GetSectree()->ForEachAround(f);

	ENTITY_MAP::iterator it, this_it;

	//

	//
	if (m_bObserverModeChange)
	{
		if (m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;
				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;


					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);


					ent->ViewRemove(this, false);
				}
				else
				{

					LPENTITY ent = this_it->first;


					//ent->EncodeRemovePacket(this);
					//m_map_view.erase(this_it);


					//ent->ViewRemove(this, false);
					EncodeRemovePacket(ent);
				}
			}
		}
		else
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;


					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);


					ent->ViewRemove(this, false);
				}
				else
				{
					LPENTITY ent = this_it->first;
					ent->EncodeInsertPacket(this);
					EncodeInsertPacket(ent);

					ent->ViewInsert(this, true);
				}
			}
		}

		m_bObserverModeChange = false;
	}
	else
	{
		if (!m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;


					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);


					ent->ViewRemove(this, false);
				}
			}
		}
	}
}
//martysama0134's 2022
