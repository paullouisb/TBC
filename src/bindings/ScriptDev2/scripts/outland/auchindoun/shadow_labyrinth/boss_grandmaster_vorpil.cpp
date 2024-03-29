/* Copyright (C) 2006 - 2011 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Grandmaster_Vorpil
SD%Complete: 75
SDComment: Despawn all summoned on death not implemented. Void Traveler effects not implemented.
SDCategory: Auchindoun, Shadow Labyrinth
EndScriptData */

#include "precompiled.h"
#include "shadow_labyrinth.h"

#define SAY_INTRO                       -1555028
#define SAY_AGGRO1                      -1555029
#define SAY_AGGRO2                      -1555030
#define SAY_AGGRO3                      -1555031
#define SAY_HELP                        -1555032
#define SAY_SLAY1                       -1555033
#define SAY_SLAY2                       -1555034
#define SAY_DEATH                       -1555035

#define SPELL_DRAW_SHADOWS              33563
#define SPELL_VOID_PORTAL_A             33566               //spell only summon one unit, but we use it for the visual effect and summon the 4 other portals manual way(only one spell exist)
#define SPELL_VOID_PORTAL_VISUAL        33569
#define SPELL_SHADOW_BOLT_VOLLEY        32963
#define SPELL_SUMMON_VOIDWALKER_A       33582
#define SPELL_SUMMON_VOIDWALKER_B       33583
#define SPELL_SUMMON_VOIDWALKER_C       33584
#define SPELL_SUMMON_VOIDWALKER_D       33585
#define SPELL_SUMMON_VOIDWALKER_E       33586
#define SPELL_RAIN_OF_FIRE              33617
#define H_SPELL_RAIN_OF_FIRE            39363
#define H_SPELL_BANISH                  38791

#define ENTRY_VOID_PORTAL               19224
#define ENTRY_VOID_TRAVELER             19226

#define LOCX                            -253.06f
#define LOCY                            -264.02f
#define LOCZ                            17.08f

struct MANGOS_DLL_DECL boss_grandmaster_vorpilAI : public ScriptedAI
{
    boss_grandmaster_vorpilAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Intro = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;

    uint32 ShadowBoltVolley_Timer;
    uint32 DrawShadows_Timer;
    uint32 Teleport_Timer;
    uint32 VoidTraveler_Timer;
    uint32 Banish_Timer;
    bool Intro;
    bool Teleport;
    bool b_yelledForHelp;

    void Reset()
    {
        ShadowBoltVolley_Timer = urand(7000, 14000);
        DrawShadows_Timer = 40000;
        Teleport_Timer = 1000;
        VoidTraveler_Timer = 20000;
        Banish_Timer = 25000;
        Teleport = false;
        b_yelledForHelp = false;

		//despawn adds between tries
		std::list<Creature*> m_lAdds;
		
		m_lAdds.clear();
		GetCreatureListWithEntryInGrid(m_lAdds, m_creature, ENTRY_VOID_TRAVELER, 150.0f);
	
		if(!m_lAdds.empty())
		{
			for(std::list<Creature*>::iterator iter = m_lAdds.begin(); iter != m_lAdds.end(); ++iter)
				if (*iter)
					(*iter)->ForcedDespawn();			
		}
		
		m_lAdds.clear();
		GetCreatureListWithEntryInGrid(m_lAdds, m_creature, ENTRY_VOID_PORTAL, 150.0f);
	
		if(!m_lAdds.empty())
		{
			for(std::list<Creature*>::iterator iter = m_lAdds.begin(); iter != m_lAdds.end(); ++iter)
				if (*iter)
					(*iter)->ForcedDespawn();			
		}
    }

    void MoveInLineOfSight(Unit *who)
    {
        //not sure about right radius
        if (!Intro && m_creature->IsWithinDistInMap(who, 40))
        {
            DoScriptText(SAY_INTRO, m_creature);
            Intro = true;
        }

        ScriptedAI::MoveInLineOfSight(who);
    }

    void Aggro(Unit *who)
    {
        switch(urand(0, 2))
        {
            case 0: DoScriptText(SAY_AGGRO1, m_creature); break;
            case 1: DoScriptText(SAY_AGGRO2, m_creature); break;
            case 2: DoScriptText(SAY_AGGRO3, m_creature); break;
        }

        DoCastSpellIfCan(m_creature, SPELL_VOID_PORTAL_A, true);
        m_creature->SummonCreature(ENTRY_VOID_PORTAL, -262.40f, -229.57f, 17.08f, 0.0f, TEMPSUMMON_CORPSE_DESPAWN,0);
        m_creature->SummonCreature(ENTRY_VOID_PORTAL, -260.35f, -297.56f, 17.08f, 0.0f, TEMPSUMMON_CORPSE_DESPAWN,0);
        m_creature->SummonCreature(ENTRY_VOID_PORTAL, -292.05f, -270.37f, 12.68f, 0.0f, TEMPSUMMON_CORPSE_DESPAWN,0);
        m_creature->SummonCreature(ENTRY_VOID_PORTAL, -301.64f, -255.97f, 12.68f, 0.0f, TEMPSUMMON_CORPSE_DESPAWN,0);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VORPIL, IN_PROGRESS);
			
		m_creature->CallForHelp(VISIBLE_RANGE);
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(urand(0, 1) ? SAY_SLAY1 : SAY_SLAY2, m_creature);
    }

    void JustSummoned(Creature *summoned)
    {
        if (summoned->GetEntry() == ENTRY_VOID_TRAVELER)
            summoned->GetMotionMaster()->MoveFollow(m_creature, 1.0f, 0.0f);

        if (summoned->GetEntry() == ENTRY_VOID_PORTAL)
		{
            summoned->CastSpell(summoned, SPELL_VOID_PORTAL_VISUAL, true);
			summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
		}
    }

    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_VORPIL, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VORPIL, FAIL);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (Teleport)
        {
            if (Teleport_Timer <= diff)
            {
				m_creature->StopMoving();
				m_creature->NearTeleportTo(LOCX, LOCY, LOCZ, 0.0f);
				m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());

                float ranX = LOCX;
                float ranY = LOCY;
                float ranZ = LOCZ;

                std::vector<ObjectGuid> vGuids;
                m_creature->FillGuidsListFromThreatList(vGuids);
                for (std::vector<ObjectGuid>::const_iterator itr = vGuids.begin();itr != vGuids.end(); ++itr)
                {
                    Unit* target = m_creature->GetMap()->GetUnit(*itr);

                    if (target && target->GetTypeId() == TYPEID_PLAYER)
                    {
                        target->GetRandomPoint(LOCX, LOCY, LOCZ, 3.0f, ranX, ranY, ranZ);
                        DoTeleportPlayer(target, ranX, ranY, ranZ, m_creature->GetAngle(m_creature->GetPositionX(), m_creature->GetPositionY()));
                    }
                }
                Teleport = false;

				DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_RAIN_OF_FIRE : H_SPELL_RAIN_OF_FIRE);				

                Teleport_Timer = 1000;

            }else Teleport_Timer -= diff;
        }

        if (ShadowBoltVolley_Timer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHADOW_BOLT_VOLLEY);
            ShadowBoltVolley_Timer = urand(15000, 30000);
        }else ShadowBoltVolley_Timer -= diff;

        if (DrawShadows_Timer < diff)
        {
            DoCastSpellIfCan(m_creature, SPELL_DRAW_SHADOWS);
            DrawShadows_Timer = 30000;
            Teleport = true;
        }else DrawShadows_Timer -= diff;

        if (VoidTraveler_Timer < diff)
        {
            if (!b_yelledForHelp)
            {
                DoScriptText(SAY_HELP, m_creature);
                b_yelledForHelp = true;
            }

            switch(urand(0, 4))
            {
                case 0: DoCastSpellIfCan(m_creature, SPELL_SUMMON_VOIDWALKER_A, CAST_TRIGGERED); break;
                case 1: DoCastSpellIfCan(m_creature, SPELL_SUMMON_VOIDWALKER_B, CAST_TRIGGERED); break;
                case 2: DoCastSpellIfCan(m_creature, SPELL_SUMMON_VOIDWALKER_C, CAST_TRIGGERED); break;
                case 3: DoCastSpellIfCan(m_creature, SPELL_SUMMON_VOIDWALKER_D, CAST_TRIGGERED); break;
                case 4: DoCastSpellIfCan(m_creature, SPELL_SUMMON_VOIDWALKER_E, CAST_TRIGGERED); break;
            }
            //faster rate when below (X) health?
			float timer = (float)(m_creature->GetHealth() / m_creature->GetMaxHealth()) * 22000.0f;
			VoidTraveler_Timer = 5000 + timer;
        }else VoidTraveler_Timer -= diff;

        if (!m_bIsRegularMode)
        {
            if (Banish_Timer < diff)
            {
                if (Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
                    DoCastSpellIfCan(target,H_SPELL_BANISH);
                Banish_Timer = 35000;
            }else Banish_Timer -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_grandmaster_vorpil(Creature* pCreature)
{
    return new boss_grandmaster_vorpilAI(pCreature);
}

enum
{
	SPELL_EMPOWERING_SHADOWS		= 33783,
	SPELL_EMPOWERING_SHADOWS_H		= 39364,
	SPELL_SHADOW_NOVA				= 33846
};
struct MANGOS_DLL_DECL mob_void_travelerAI : public ScriptedAI
{
    mob_void_travelerAI(Creature* pCreature) : ScriptedAI(pCreature) 
	{
		m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
		Reset();
	}
	
    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;
	
	uint32 Check_Timer;
	
    void Reset() 
	{
		Check_Timer = 2000;
	}

    void MoveInLineOfSight(Unit *who)  { return; }

	void AttackStart(Unit* pWho) { return; }
	
    void UpdateAI(const uint32 diff)   
	{
		if (Check_Timer < diff)
		{
			if (Creature* pVorpil = m_pInstance->GetSingleCreatureFromStorage(NPC_VORPIL))
				if(m_creature->IsWithinDistInMap(pVorpil, 10.0f))
				{
					DoCastSpellIfCan(m_creature, SPELL_SHADOW_NOVA);
                    if (pVorpil->isAlive())
						pVorpil->AI()->DoCastSpellIfCan(pVorpil, m_bIsRegularMode ? SPELL_EMPOWERING_SHADOWS : SPELL_EMPOWERING_SHADOWS_H, true);
                    m_creature->ForcedDespawn();
				}
			Check_Timer = 2000;
		}else Check_Timer -= diff;
	}

};

CreatureAI* GetAI_mob_void_traveler(Creature* pCreature)
{
    return new mob_void_travelerAI(pCreature);
}

void AddSC_boss_grandmaster_vorpil()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_grandmaster_vorpil";
    newscript->GetAI = &GetAI_boss_grandmaster_vorpil;
    newscript->RegisterSelf();
	
    newscript = new Script;
    newscript->Name = "mob_void_traveler";
    newscript->GetAI = &GetAI_mob_void_traveler;
    newscript->RegisterSelf();
}
