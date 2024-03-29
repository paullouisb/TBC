/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
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
SDName: Black_Temple
SD%Complete: 95
SDComment: Spirit of Olum: Player Teleporter to Seer Kanai Teleport after defeating Naj'entus and Supremus. TODO: Find proper gossip.
SDCategory: Black Temple
EndScriptData */

/* ContentData
npc_spirit_of_olum
EndContentData */

#include "precompiled.h"
#include "black_temple.h"

/*###
# npc_spirit_of_olum
####*/

#define SPELL_TELEPORT_ASHTONGUE   41566                           // s41566 - Teleport to Ashtongue NPC's
#define GOSSIP_OLUM1               "Teleport me to the other Ashtongue Deathsworn"
#define SPELL_TELEPORT_COUNCIL     41570                           // s41570 - Teleport to Council
#define GOSSIP_OLUM2               "Teleport me to the Chamber of Command"

bool GossipHello_npc_spirit_of_olum(Player* pPlayer, Creature* pCreature)
{
	ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

	if (pInstance)
	{
		if((pInstance->GetData(TYPE_SUPREMUS) >= DONE) && (pInstance->GetData(TYPE_NAJENTUS) >= DONE))
			pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_OLUM1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
		if(pInstance->GetData(TYPE_COUNCIL) >= DONE)
			pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_OLUM2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);		
	}

	pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());
	return true;
}

bool GossipSelect_npc_spirit_of_olum(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
{
	if (uiAction == GOSSIP_ACTION_INFO_DEF + 1 || uiAction == GOSSIP_ACTION_INFO_DEF + 2)
		pPlayer->CLOSE_GOSSIP_MENU();

	pPlayer->InterruptNonMeleeSpells(false);
	if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
		pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_ASHTONGUE, false);
	else if(uiAction == GOSSIP_ACTION_INFO_DEF + 2)
		pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_COUNCIL, false);

	return true;
}

void AddSC_black_temple()
{
	Script* pNewScript;

	pNewScript = new Script;
	pNewScript->Name = "npc_spirit_of_olum";
	pNewScript->pGossipHello = &GossipHello_npc_spirit_of_olum;
	pNewScript->pGossipSelect = &GossipSelect_npc_spirit_of_olum;
	pNewScript->RegisterSelf();
}
