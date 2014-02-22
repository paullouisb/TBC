/**
* This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
*
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/Singleton.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround/BattleGround.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "MapManager.h"

#define NULL_AURA_SLOT 0xFF

/**
* An array with all the different handlers for taking care of
* the various aura types that are defined in AuraType.
*/
pAuraHandler AuraHandler[TOTAL_AURAS] =
{
	&Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
	&Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
	&Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
	&Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
	&Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
	&Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
	&Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
	&Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
	&Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
	&Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
	&Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
	&Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
	&Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
	&Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
	&Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonusTaken and Unit::SpellBaseDamageBonusTaken
	&Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD      implemented in Unit::DealMeleeDamage
	&Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
	&Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT implemented in Unit::isVisibleForOrDetect
	&Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
	&Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
	&Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
	&Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
	&Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
	&Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
	&Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
	&Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
	&Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
	&Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
	&Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
	&Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
	&Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
	&Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
	&Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
	&Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
	&Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
	&Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
	&Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
	&Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
	&Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
	&Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
	&Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
	&Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
	&Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
	&Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
	&Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
	&Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
	&Aura::HandleUnused,                                    // 46 SPELL_AURA_46
	&Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
	&Aura::HandleUnused,                                    // 48 SPELL_AURA_48
	&Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
	&Aura::HandleUnused,                                    // 50 SPELL_AURA_MOD_BLOCK_SKILL    obsolete?
	&Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
	&Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
	&Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
	&Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
	&Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
	&Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
	&Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
	&Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
	&Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonusDone and Unit::SpellDamageBonusDone
	&Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
	&Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
	&Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
	&Aura::HandleUnused,                                    // 63 SPELL_AURA_PERIODIC_MANA_FUNNEL obsolete?
	&Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
	&Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
	&Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
	&Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
	&Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
	&Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalculateAbsorbAndResist
	&Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell that has only visual effect
	&Aura::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
	&Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
	&Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
	&Aura::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
	&Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE           implemented in WorldSession::HandleMessagechatOpcode
	&Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
	&Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
	&Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
	&Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
	&Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
	&Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalculateAbsorbAndResist
	&Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
	&Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
	&Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
	&Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
	&Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
	&Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonusTaken and Unit::SpellDamageBonusTaken
	&Aura::HandleModHealthRegenPCT,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
	&Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
	&Aura::HandleUnused,                                    // 90 SPELL_AURA_MOD_RESIST_CHANCE  Useless
	&Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
	&Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
	&Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
	&Aura::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
	&Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
	&Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
	&Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalculateAbsorbAndResist
	&Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
	&Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
	&Aura::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete? all player can see all auras now
	&Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
	&Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
	&Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
	&Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
	&Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
	&Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
	&Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
	&Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
	&Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
	&Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
	&Aura::HandleNoImmediateEffect,                         //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER implemented in Unit::SelectMagnetTarget
	&Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
	&Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
	&Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
	&Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
	&Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
	&Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonusTaken
	&Aura::HandleUnused,                                    //119 SPELL_AURA_SHARE_PET_TRACKING useless
	&Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
	&Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
	&Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
	&Aura::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
	&Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
	&Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
	&Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
	&Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
	&Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
	&Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
	&Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
	&Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
	&Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
	&Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
	&Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
	&Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
	&Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonusDone
	&Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
	&Aura::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
	&Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
	&Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
	&Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
	&Aura::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
	&Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
	&Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                  implemented in WorldSession::HandleMovementOpcodes
	&Aura::HandleUnused,                                    //145 SPELL_AURA_CHARISMA obsolete?
	&Aura::HandleUnused,                                    //146 SPELL_AURA_PERSUADED obsolete?
	&Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK     implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect (check part)
	&Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
	&Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_RESIST_PUSHBACK            implemented in Spell::Delayed and Spell::DelayedChannel
	&Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
	&Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
	&Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE         implemented in Creature::GetAttackDistance
	&Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalculateAbsorbAndResist
	&Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
	&Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::getMaxTimer
	&Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
	&Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
	&Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
	&Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT              implemented in Player::RewardHonor
	&Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE          implemented in Unit::MagicSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT implemented in Player::RegenerateAll and Player::RegenerateHealth
	&Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
	&Aura::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS      implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
	&Aura::HandleUnused,                                    //164 useless, only one test spell
	&Aura::HandleModMeleeAttackPowerAttackerBonus,          //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
	&Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
	&Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
	&Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonusDone, Unit::MeleeDamageBonusDone
	&Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
	&Aura::HandleDetectAmore,                               //170 SPELL_AURA_DETECT_AMORE       only for Detect Amore spell
	&Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
	&Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
	&Aura::HandleUnused,                                    //173 SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
	&Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonusDone
	&Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonusDone
	&Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
	&Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM
	&Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
	&Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonusDone
	&Aura::HandleUnused,                                    //181 SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS unused
	&Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
	&Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::CalcThreat
	&Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
	&Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
	&Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
	&Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
	&Aura::HandleModRating,                                 //189 SPELL_AURA_MOD_RATING
	&Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
	&Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
	&Aura::HandleModMeleeRangedSpeedPct,                    //192 SPELL_AURA_MOD_MELEE_RANGED_HASTE
	&Aura::HandleModCombatSpeedPct,                         //193 SPELL_AURA_HASTE_ALL (in fact combat (any type attack) speed pct)
	&Aura::HandleUnused,                                    //194 SPELL_AURA_MOD_DEPRICATED_1 not used now (old SPELL_AURA_MOD_SPELL_DAMAGE_OF_INTELLECT)
	&Aura::HandleUnused,                                    //195 SPELL_AURA_MOD_DEPRICATED_2 not used now (old SPELL_AURA_MOD_SPELL_HEALING_OF_INTELLECT)
	&Aura::HandleNULL,                                      //196 SPELL_AURA_MOD_COOLDOWN
	&Aura::HandleNoImmediateEffect,                         //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE implemented in Unit::SpellCriticalBonus Unit::GetUnitCriticalChance
	&Aura::HandleUnused,                                    //198 SPELL_AURA_MOD_ALL_WEAPON_SKILLS
	&Aura::HandleNoImmediateEffect,                         //199 SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT  implemented in Unit::MagicSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //200 SPELL_AURA_MOD_XP_PCT                      implemented in Player::GiveXP
	&Aura::HandleAuraAllowFlight,                           //201 SPELL_AURA_FLY                             this aura enable flight mode...
	&Aura::HandleNoImmediateEffect,                         //202 SPELL_AURA_IGNORE_COMBAT_RESULT            implemented in Unit::MeleeSpellHitResult
	&Aura::HandleNoImmediateEffect,                         //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE  implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
	&Aura::HandleNoImmediateEffect,                         //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
	&Aura::HandleNoImmediateEffect,                         //205 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE  implemented in Unit::SpellCriticalDamageBonus
	&Aura::HandleAuraModIncreaseFlightSpeed,                //206 SPELL_AURA_MOD_FLIGHT_SPEED
	&Aura::HandleAuraModIncreaseFlightSpeed,                //207 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED
	&Aura::HandleAuraModIncreaseFlightSpeed,                //208 SPELL_AURA_MOD_FLIGHT_SPEED_STACKING
	&Aura::HandleAuraModIncreaseFlightSpeed,                //209 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING
	&Aura::HandleAuraModIncreaseFlightSpeed,                //210 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING
	&Aura::HandleAuraModIncreaseFlightSpeed,                //211 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING
	&Aura::HandleAuraModRangedAttackPowerOfStatPercent,     //212 SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
	&Aura::HandleNoImmediateEffect,                         //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
	&Aura::HandleNULL,                                      //214 Tamed Pet Passive
	&Aura::HandleArenaPreparation,                          //215 SPELL_AURA_ARENA_PREPARATION
	&Aura::HandleModCastingSpeed,                           //216 SPELL_AURA_HASTE_SPELLS
	&Aura::HandleUnused,                                    //217                                   unused
	&Aura::HandleAuraModRangedHaste,                        //218 SPELL_AURA_HASTE_RANGED
	&Aura::HandleModManaRegen,                              //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
	&Aura::HandleUnused,                                    //220 SPELL_AURA_MOD_RATING_FROM_STAT
	&Aura::HandleNULL,                                      //221 ignored
	&Aura::HandleUnused,                                    //222 unused
	&Aura::HandleNULL,                                      //223 Cold Stare
	&Aura::HandleUnused,                                    //224 unused
	&Aura::HandleNoImmediateEffect,                         //225 SPELL_AURA_PRAYER_OF_MENDING
	&Aura::HandleAuraPeriodicDummy,                         //226 SPELL_AURA_PERIODIC_DUMMY
	&Aura::HandlePeriodicTriggerSpellWithValue,             //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE
	&Aura::HandleNoImmediateEffect,                         //228 SPELL_AURA_DETECT_STEALTH
	&Aura::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE        implemented in Unit::SpellDamageBonusTaken
	&Aura::HandleAuraModIncreaseMaxHealth,                  //230 Commanding Shout
	&Aura::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
	&Aura::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateAuraDuration
	&Aura::HandleNULL,                                      //233 set model id to the one of the creature with id m_modifier.m_miscvalue
	&Aura::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateAuraDuration
	&Aura::HandleAuraModDispelResist,                       //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
	&Aura::HandleUnused,                                    //236 unused
	&Aura::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonusDone
	&Aura::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonusDone
	&Aura::HandleAuraModScale,                              //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
	&Aura::HandleAuraModExpertise,                          //240 SPELL_AURA_MOD_EXPERTISE
	&Aura::HandleForceMoveForward,                          //241 Forces the caster to move forward
	&Aura::HandleUnused,                                    //242 unused
	&Aura::HandleUnused,                                    //243 used by two test spells
	&Aura::HandleComprehendLanguage,                        //244 SPELL_AURA_COMPREHEND_LANGUAGE
	&Aura::HandleUnused,                                    //245 unused
	&Aura::HandleUnused,                                    //246 unused
	&Aura::HandleAuraMirrorImage,                           //247 SPELL_AURA_MIRROR_IMAGE                      target to become a clone of the caster
	&Aura::HandleNoImmediateEffect,                         //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE         implemented in Unit::RollMeleeOutcomeAgainst
	&Aura::HandleNULL,                                      //249
	&Aura::HandleAuraModIncreaseHealth,                     //250 SPELL_AURA_MOD_INCREASE_HEALTH_2
	&Aura::HandleNULL,                                      //251 SPELL_AURA_MOD_ENEMY_DODGE
	&Aura::HandleUnused,                                    //252 unused
	&Aura::HandleUnused,                                    //253 unused
	&Aura::HandleUnused,                                    //254 unused
	&Aura::HandleUnused,                                    //255 unused
	&Aura::HandleUnused,                                    //256 unused
	&Aura::HandleUnused,                                    //257 unused
	&Aura::HandleUnused,                                    //258 unused
	&Aura::HandleUnused,                                    //259 unused
	&Aura::HandleUnused,                                    //260 unused
	&Aura::HandleNULL                                       //261 SPELL_AURA_261 some phased state (44856 spell)
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32* currentBasePoints, SpellAuraHolder* holder, Unit* target, Unit* caster, Item* castItem) :
	m_spellmod(NULL), m_periodicTimer(0), m_periodicTick(0), m_removeMode(AURA_REMOVE_BY_DEFAULT),
	m_effIndex(eff), m_positive(false), m_isPeriodic(false), m_isAreaAura(false),
	m_isPersistent(false), m_in_use(0), m_spellAuraHolder(holder)
{
	MANGOS_ASSERT(target);
	MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry(spellproto->Id) && "`info` must be pointer to sSpellStore element");

	m_currentBasePoints = currentBasePoints ? *currentBasePoints : spellproto->CalculateSimpleValue(eff);

	m_positive = IsPositiveEffect(spellproto, m_effIndex);
	m_applyTime = time(NULL);

	int32 damage;
	if (!caster)
		damage = m_currentBasePoints;
	else
	{
		damage = caster->CalculateSpellDamage(target, spellproto, m_effIndex, &m_currentBasePoints);

		if (!damage && castItem && castItem->GetItemSuffixFactor())
		{
			ItemRandomSuffixEntry const* item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(castItem->GetItemRandomPropertyId()));
			if (item_rand_suffix)
			{
				for (int k = 0; k < 3; ++k)
				{
					SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(item_rand_suffix->enchant_id[k]);
					if (pEnchant)
					{
						for (int t = 0; t < 3; ++t)
						{
							if (pEnchant->spellid[t] != spellproto->Id)
								continue;

							damage = uint32((item_rand_suffix->prefix[k] * castItem->GetItemSuffixFactor()) / 10000);
							break;
						}
					}

					if (damage)
						break;
				}
			}
		}
	}

	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura: construct Spellid : %u, Aura : %u Target : %d Damage : %d", spellproto->Id, spellproto->EffectApplyAuraName[eff], spellproto->EffectImplicitTargetA[eff], damage);


	// Apply haste for aura triggered by channeled cast @Kordbc
	uint32 pt = spellproto->EffectAmplitude[eff];
	if(caster && (spellproto->HasAttribute(SPELL_ATTR_EX_CHANNELED_1) || spellproto->HasAttribute(SPELL_ATTR_EX_CHANNELED_2)))
		pt = uint32(float(pt) * caster->GetFloatValue(UNIT_MOD_CAST_SPEED));

	SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, pt, spellproto->EffectMiscValue[eff]);

	Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

	// Apply periodic time mod
	if (modOwner && m_modifier.periodictime)
		modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);

	// Start periodic on next tick or at aura apply
	if (!spellproto->HasAttribute(SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY))
		m_periodicTimer = m_modifier.periodictime;
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32* currentBasePoints, SpellAuraHolder* holder, Unit* target,
				   Unit* caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
	m_isAreaAura = true;

	// caster==NULL in constructor args if target==caster in fact
	Unit* caster_ptr = caster ? caster : target;

	m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellproto->EffectRadiusIndex[m_effIndex]));
	if (Player* modOwner = caster_ptr->GetSpellModOwner())
		modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_RADIUS, m_radius);

	switch (spellproto->Effect[eff])
	{
	case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
		m_areaAuraType = AREA_AURA_PARTY;
		break;
	case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
		m_areaAuraType = AREA_AURA_FRIEND;
		break;
	case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
		m_areaAuraType = AREA_AURA_ENEMY;
		if (target == caster_ptr)
			m_modifier.m_auraname = SPELL_AURA_NONE;    // Do not do any effect on self
		break;
	case SPELL_EFFECT_APPLY_AREA_AURA_PET:
		m_areaAuraType = AREA_AURA_PET;
		break;
	case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
		m_areaAuraType = AREA_AURA_OWNER;
		if (target == caster_ptr)
			m_modifier.m_auraname = SPELL_AURA_NONE;
		break;
	default:
		sLog.outError("Wrong spell effect in AreaAura constructor");
		MANGOS_ASSERT(false);
		break;
	}

	// totems are immune to any kind of area auras
	if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
		m_modifier.m_auraname = SPELL_AURA_NONE;
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32* currentBasePoints, SpellAuraHolder* holder, Unit* target,
									   Unit* caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
	m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32* currentBasePoints, SpellAuraHolder* holder, Unit* target,
											 Unit* caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
	if (caster)
		m_castersTargetGuid = caster->GetTypeId() == TYPEID_PLAYER ? ((Player*)caster)->GetSelectionGuid() : caster->GetTargetGuid();
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
	return ObjectAccessor::GetUnit(*(m_spellAuraHolder->GetTarget()), m_castersTargetGuid);
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32* currentBasePoints, SpellAuraHolder* holder, Unit* target, Unit* caster, Item* castItem)
{
	if (IsAreaAuraEffect(spellproto->Effect[eff]))
		return new AreaAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

	uint32 triggeredSpellId = spellproto->EffectTriggerSpell[eff];

	if (SpellEntry const* triggeredSpellInfo = sSpellStore.LookupEntry(triggeredSpellId))
		for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
			if (triggeredSpellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_ENEMY)
				return new SingleEnemyTargetAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

	return new Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);
}

SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, Unit* target, WorldObject* caster, Item* castItem)
{
	return new SpellAuraHolder(spellproto, target, caster, castItem);
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue)
{
	m_modifier.m_auraname = t;
	m_modifier.m_amount = a;
	m_modifier.m_miscvalue = miscValue;
	m_modifier.periodictime = pt;
}

void Aura::Update(uint32 diff)
{
	if (m_isPeriodic)
	{
		m_periodicTimer -= diff;
		if (m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
		{
			// update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
			m_periodicTimer += m_modifier.periodictime;
			++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
			PeriodicTick();
		}
	}
}

void AreaAura::Update(uint32 diff)
{
	// update for the caster of the aura
	if (GetCasterGuid() == GetTarget()->GetObjectGuid())
	{
		Unit* caster = GetTarget();

		if (!caster->hasUnitState(UNIT_STAT_ISOLATED))
		{
			Unit* owner = caster->GetCharmerOrOwner();
			if (!owner)
				owner = caster;
			Spell::UnitList targets;

			switch (m_areaAuraType)
			{
			case AREA_AURA_PARTY:
				{
					Group* pGroup = NULL;

					if (owner->GetTypeId() == TYPEID_PLAYER)
						pGroup = ((Player*)owner)->GetGroup();

					if (pGroup)
					{
						uint8 subgroup = ((Player*)owner)->GetSubGroup();
						for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
						{
							Player* Target = itr->getSource();
							if (Target && Target->isAlive() && Target->GetSubGroup() == subgroup && caster->IsFriendlyTo(Target))
							{
								if (caster->IsWithinDistInMap(Target, m_radius))
									targets.push_back(Target);
								Pet* pet = Target->GetPet();
								if (pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
									targets.push_back(pet);
							}
						}
					}
					else
					{
						// add owner
						if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
							targets.push_back(owner);
						// add caster's pet
						Unit* pet = caster->GetPet();
						if (pet && caster->IsWithinDistInMap(pet, m_radius))
							targets.push_back(pet);
					}
					break;
				}
			case AREA_AURA_FRIEND:
				{
					MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(caster, m_radius);
					MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(targets, u_check);
					Cell::VisitAllObjects(caster, searcher, m_radius);
					break;
				}
			case AREA_AURA_ENEMY:
				{
					MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(caster, m_radius); // No GetCharmer in searcher
					MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(targets, u_check);
					Cell::VisitAllObjects(caster, searcher, m_radius);
					break;
				}
			case AREA_AURA_OWNER:
			case AREA_AURA_PET:
				{
					if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
						targets.push_back(owner);
					break;
				}
			}

			for (Spell::UnitList::iterator tIter = targets.begin(); tIter != targets.end(); ++tIter)
			{
				// flag for seelction is need apply aura to current iteration target
				bool apply = true;

				// we need ignore present caster self applied are auras sometime
				// in cases if this only auras applied for spell effect
				Unit::SpellAuraHolderBounds spair = (*tIter)->GetSpellAuraHolderBounds(GetId());
				for (Unit::SpellAuraHolderMap::const_iterator i = spair.first; i != spair.second; ++i)
				{
					if (i->second->IsDeleted())
						continue;

					Aura* aur = i->second->GetAuraByEffectIndex(m_effIndex);

					if (!aur)
						continue;

					switch (m_areaAuraType)
					{
					case AREA_AURA_ENEMY:
						// non caster self-casted auras (non stacked)
						if (aur->GetModifier()->m_auraname != SPELL_AURA_NONE)
							apply = false;
						break;
					default:
						// in generic case not allow stacking area auras
						apply = false;
						break;
					}

					if (!apply)
						break;
				}

				if (!apply)
					continue;

				// Skip some targets (TODO: Might require better checks, also unclear how the actual caster must/can be handled)
				if (GetSpellProto()->HasAttribute(SPELL_ATTR_EX3_TARGET_ONLY_PLAYER) && (*tIter)->GetTypeId() != TYPEID_PLAYER)
					continue;

				if (SpellEntry const* actualSpellInfo = sSpellMgr.SelectAuraRankForLevel(GetSpellProto(), (*tIter)->getLevel()))
				{
					int32 actualBasePoints = m_currentBasePoints;
					// recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
					if (actualSpellInfo != GetSpellProto())
						actualBasePoints = actualSpellInfo->CalculateSimpleValue(m_effIndex);

					SpellAuraHolder* holder = (*tIter)->GetSpellAuraHolder(actualSpellInfo->Id, GetCasterGuid());

					bool addedToExisting = true;
					if (!holder)
					{
						holder = CreateSpellAuraHolder(actualSpellInfo, (*tIter), caster);
						addedToExisting = false;
					}

					holder->SetAuraDuration(GetAuraDuration());

					AreaAura* aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, holder, (*tIter), caster, NULL);
					holder->AddAura(aur, m_effIndex);

					if (addedToExisting)
					{
						(*tIter)->AddAuraToModList(aur);
						holder->SetInUse(true);
						aur->ApplyModifier(true, true);
						holder->SetInUse(false);
					}
					else
						(*tIter)->AddSpellAuraHolder(holder);
				}
			}
		}
		Aura::Update(diff);
	}
	else                                                    // aura at non-caster
	{
		Unit* caster = GetCaster();
		Unit* target = GetTarget();

		Aura::Update(diff);

		// remove aura if out-of-range from caster (after teleport for example)
		// or caster is isolated or caster no longer has the aura
		// or caster is (no longer) friendly
		bool needFriendly = (m_areaAuraType == AREA_AURA_ENEMY ? false : true);
		if (!caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
			!caster->IsWithinDistInMap(target, m_radius)        ||
			!caster->HasAura(GetId(), GetEffIndex())            ||
			caster->IsFriendlyTo(target) != needFriendly
			)
		{
			target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
		}
		else if (m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
		{
			// not check group if target == owner or target == pet
			if (caster->GetCharmerOrOwnerGuid() != target->GetObjectGuid() && caster->GetObjectGuid() != target->GetCharmerOrOwnerGuid())
			{
				Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

				Group* pGroup = check ? check->GetGroup() : NULL;
				if (pGroup)
				{
					Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
					if (!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
						target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
				}
				else
					target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
			}
		}
		else if (m_areaAuraType == AREA_AURA_PET || m_areaAuraType == AREA_AURA_OWNER)
		{
			if (target->GetObjectGuid() != caster->GetCharmerOrOwnerGuid())
				target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
		}
	}
}

void PersistentAreaAura::Update(uint32 diff)
{
	bool remove = false;

	// remove the aura if its caster or the dynamic object causing it was removed
	// or if the target moves too far from the dynamic object
	if (Unit* caster = GetCaster())
	{
		DynamicObject* dynObj = caster->GetDynObject(GetId(), GetEffIndex());
		if (dynObj)
		{
			if (!GetTarget()->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
			{
				remove = true;
				dynObj->RemoveAffected(GetTarget());        // let later reapply if target return to range
			}
		}
		else
			remove = true;
	}
	else
		remove = true;

	Aura::Update(diff);

	if (remove)
		GetTarget()->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real)
{
	AuraType aura = m_modifier.m_auraname;

	GetHolder()->SetInUse(true);
	SetInUse(true);
	if (aura < TOTAL_AURAS)
		(*this.*AuraHandler [aura])(apply, Real);
	SetInUse(false);
	GetHolder()->SetInUse(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const* spell) const
{
	if (m_spellmod)
		return m_spellmod->isAffectedOnSpell(spell);

	// Check family name
	if (spell->SpellFamilyName != GetSpellProto()->SpellFamilyName)
		return false;

	ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());
	return spell->IsFitToFamilyMask(mask);
}

bool Aura::CanProcFrom(SpellEntry const* spell, uint32 EventProcEx, uint32 procEx, bool active, bool useClassMask) const
{
	// Check EffectClassMask (in pre-3.x stored in spell_affect in fact)
	ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());

	// if no class mask defined, or spell_proc_event has SpellFamilyName=0 - allow proc
	if (!useClassMask || !mask)
	{
		if (!(EventProcEx & PROC_EX_EX_TRIGGER_ALWAYS))
		{
			// Check for extra req (if none) and hit/crit
			if (EventProcEx == PROC_EX_NONE)
			{
				// No extra req, so can trigger only for active (damage/healing present) and hit/crit
				if ((procEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)) && active)
					return true;
				else
				{
					for(int i = 0; i < MAX_EFFECT_INDEX; i++)
					{
						switch(spell->EffectApplyAuraName[i])
						{
							// Some aura effect can proc when !active
						case SPELL_AURA_PERIODIC_DAMAGE:
						case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
						case SPELL_AURA_PERIODIC_HEAL:
						case SPELL_AURA_PERIODIC_LEECH:
						case SPELL_AURA_MOD_FEAR:
						case SPELL_AURA_DUMMY:
							return true;
							break;
						default:break;
						}
					}
					return false;
				}
			}
			else // Passive spells hits here only if resist/reflect/immune/evade
			{
				// Passive spells can`t trigger if need hit (exclude cases when procExtra include non-active flags)
				if ((EventProcEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT) & procEx) && !active)
					return false;
			}
		}
		return true;
	}
	else
	{
		// SpellFamilyName check is performed in SpellMgr::IsSpellProcEventCanTriggeredBy and it is done once for whole holder
		// note: SpellFamilyName is not checked if no spell_proc_event is defined
		return mask.IsFitToFamilyMask(spell->SpellFamilyFlags);
	}
}

void Aura::ReapplyAffectedPassiveAuras(Unit* target, bool owner_mode)
{
	// we need store cast item guids for self casted spells
	// expected that not exist permanent auras from stackable auras from different items
	std::map<uint32, ObjectGuid> affectedSelf;

	std::set<uint32> affectedAuraCaster;

	for (Unit::SpellAuraHolderMap::const_iterator itr = target->GetSpellAuraHolderMap().begin(); itr != target->GetSpellAuraHolderMap().end(); ++itr)
	{
		// permanent passive or permanent area aura
		// passive spells can be affected only by own or owner spell mods)
		if ((itr->second->IsPermanent() && ((owner_mode && itr->second->IsPassive()) || itr->second->IsAreaAura())) &&
			// non deleted and not same aura (any with same spell id)
				!itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
				// and affected by aura
				isAffectedOnSpell(itr->second->GetSpellProto()))
		{
			// only applied by self or aura caster
			if (itr->second->GetCasterGuid() == target->GetObjectGuid())
				affectedSelf[itr->second->GetId()] = itr->second->GetCastItemGuid();
			else if (itr->second->GetCasterGuid() == GetCasterGuid())
				affectedAuraCaster.insert(itr->second->GetId());
		}
	}

	if (!affectedSelf.empty())
	{
		Player* pTarget = target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : NULL;

		for (std::map<uint32, ObjectGuid>::const_iterator map_itr = affectedSelf.begin(); map_itr != affectedSelf.end(); ++map_itr)
		{
			Item* item = pTarget && map_itr->second ? pTarget->GetItemByGuid(map_itr->second) : NULL;
			target->RemoveAurasDueToSpell(map_itr->first);
			target->CastSpell(target, map_itr->first, true, item);
		}
	}

	if (!affectedAuraCaster.empty())
	{
		Unit* caster = GetCaster();
		for (std::set<uint32>::const_iterator set_itr = affectedAuraCaster.begin(); set_itr != affectedAuraCaster.end(); ++set_itr)
		{
			target->RemoveAurasDueToSpell(*set_itr);
			if (caster)
				caster->CastSpell(GetTarget(), *set_itr, true);
		}
	}
}

struct ReapplyAffectedPassiveAurasHelper
{
	explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
	void operator()(Unit* unit) const { aura->ReapplyAffectedPassiveAuras(unit, true); }
	Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
	// not reapply spell mods with charges (use original value because processed and at remove)
	if (GetSpellProto()->procCharges)
		return;

	// not reapply some spell mods ops (mostly speedup case)
	switch (m_modifier.m_miscvalue)
	{
	case SPELLMOD_DURATION:
	case SPELLMOD_CHARGES:
	case SPELLMOD_NOT_LOSE_CASTING_TIME:
	case SPELLMOD_CASTING_TIME:
	case SPELLMOD_COOLDOWN:
	case SPELLMOD_COST:
	case SPELLMOD_ACTIVATION_TIME:
	case SPELLMOD_CASTING_TIME_OLD:
		return;
	}

	// reapply talents to own passive persistent auras
	ReapplyAffectedPassiveAuras(GetTarget(), true);

	// re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
	GetTarget()->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this), CONTROLLED_PET | CONTROLLED_TOTEMS);

	// re-apply talents/passives/area auras applied to group members (it affected by player spellmods)
	if (Group* group = ((Player*)GetTarget())->GetGroup())
		for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
			if (Player* member = itr->getSource())
				if (member != GetTarget() && member->IsInMap(GetTarget()))
					ReapplyAffectedPassiveAuras(member, false);
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
		return;

	if (m_modifier.m_miscvalue >= MAX_SPELLMOD)
		return;

	if (apply)
	{
		SpellEntry const* spellProto = GetSpellProto();

		// Add custom charges for some mod aura
		switch (spellProto->Id)
		{
		case 17941:                                     // Shadow Trance
		case 22008:                                     // Netherwind Focus
		case 34936:                                     // Backlash
			GetHolder()->SetAuraCharges(1);
			break;
		}

		m_spellmod = new SpellModifier(
			SpellModOp(m_modifier.m_miscvalue),
			SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
			m_modifier.m_amount,
			this,
			// prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
			// all this spell expected expire not at use but at spell proc event check
			spellProto->StackAmount > 1 ? 0 : GetHolder()->GetAuraCharges());
	}

	((Player*)GetTarget())->AddSpellMod(m_spellmod, apply);

	ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
	ObjectGuid casterGUID = GetCasterGuid();
	Unit* triggerTarget = GetTriggerTarget();

	if (!casterGUID || !triggerTarget)
		return;

	// generic casting code with custom spells and target/caster customs
	uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

	SpellEntry const* triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
	SpellEntry const* auraSpellInfo = GetSpellProto();
	uint32 auraId = auraSpellInfo->Id;
	Unit* target = GetTarget();
	Unit* triggerCaster = triggerTarget;
	WorldObject* triggerTargetObject = NULL;

	// specific code for cases with no trigger spell provided in field
	if (triggeredSpellInfo == NULL)
	{
		switch (auraSpellInfo->SpellFamilyName)
		{
		case SPELLFAMILY_GENERIC:
			{
				switch (auraId)
				{
					// Firestone Passive (1-5 ranks)
				case 758:
				case 17945:
				case 17947:
				case 17949:
				case 27252:
					{
						if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
							return;
						Item* item = ((Player*)triggerTarget)->GetWeaponForAttack(BASE_ATTACK);
						if (!item)
							return;
						uint32 enchant_id = 0;
						switch (GetId())
						{
						case   758: enchant_id = 1803; break;   // Rank 1
						case 17945: enchant_id = 1823; break;   // Rank 2
						case 17947: enchant_id = 1824; break;   // Rank 3
						case 17949: enchant_id = 1825; break;   // Rank 4
						case 27252: enchant_id = 2645; break;   // Rank 5
						default:
							return;
						}
						// remove old enchanting before applying new
						((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, false);
						item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime + 1000, 0);
						// add new enchanting
						((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, true);
						return;
					}
				case 812:                               // Periodic Mana Burn
					{
						trigger_spell_id = 25779;           // Mana Burn

						if (GetTarget()->GetTypeId() != TYPEID_UNIT)
							return;

						triggerTarget = ((Creature*)GetTarget())->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0, trigger_spell_id, SELECT_FLAG_POWER_MANA);
						if (!triggerTarget)
							return;

						break;
					}
					//                    // Polymorphic Ray
					//                    case 6965: break;
					//                    // Fire Nova (1-7 ranks)
					//                    case 8350:
					//                    case 8508:
					//                    case 8509:
					//                    case 11312:
					//                    case 11313:
					//                    case 25540:
					//                    case 25544:
					//                        break;
				case 9712:                              // Thaumaturgy Channel
					trigger_spell_id = 21029;
					break;
					//                    // Egan's Blaster
					//                    case 17368: break;
					//                    // Haunted
					//                    case 18347: break;
					//                    // Ranshalla Waiting
					//                    case 18953: break;
					//                    // Inferno
					//                    case 19695: break;
					//                    // Frostwolf Muzzle DND
					//                    case 21794: break;
					//                    // Alterac Ram Collar DND
					//                    case 21866: break;
					//                    // Celebras Waiting
					//                    case 21916: break;
				case 23170:                             // Brood Affliction: Bronze
					{
						target->CastSpell(target, 23171, true, NULL, this);
						return;
					}
				case 23184:                             // Mark of Frost
				case 25041:                             // Mark of Nature
				case 37125:                             // Mark of Death
					{
						std::list<Player*> targets;

						// spells existed in 1.x.x; 23183 - mark of frost; 25042 - mark of nature; both had radius of 100.0 yards in 1.x.x DBC
						// spells are used by Azuregos and the Emerald dragons in order to put a stun debuff on the players which resurrect during the encounter
						// in order to implement the missing spells we need to make a grid search for hostile players and check their auras; if they are marked apply debuff
						// spell 37127 used for the Mark of Death, is used server side, so it needs to be implemented here

						uint32 markSpellId = 0;
						uint32 debuffSpellId = 0;

						switch (auraId)
						{
						case 23184:
							markSpellId = 23182;
							debuffSpellId = 23186;
							break;
						case 25041:
							markSpellId = 25040;
							debuffSpellId = 25043;
							break;
						case 37125:
							markSpellId = 37128;
							debuffSpellId = 37131;
							break;
						}

						MaNGOS::AnyPlayerInObjectRangeWithAuraCheck u_check(GetTarget(), 100.0f, markSpellId);
						MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeWithAuraCheck > checker(targets, u_check);
						Cell::VisitWorldObjects(GetTarget(), checker, 100.0f);

						for (std::list<Player*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
							(*itr)->CastSpell((*itr), debuffSpellId, true, NULL, NULL, casterGUID);

						return;
					}
				case 23493:                             // Restoration
					{
						uint32 heal = triggerTarget->GetMaxHealth() / 10;
						triggerTarget->DealHeal(triggerTarget, heal, auraSpellInfo);

						if (int32 mana = triggerTarget->GetMaxPower(POWER_MANA))
						{
							mana /= 10;
							triggerTarget->EnergizeBySpell(triggerTarget, 23493, mana, POWER_MANA);
						}
						return;
					}
					//                    // Stoneclaw Totem Passive TEST
					//                    case 23792: break;
					//                    // Axe Flurry
					//                    case 24018: break;
				case 24210:                             // Mark of Arlokk
					{
						// Replacement for (classic) spell 24211 (doesn't exist anymore)
						std::list<Creature*> lList;

						// Search for all Zulian Prowler in range
						MaNGOS::AllCreaturesOfEntryInRangeCheck check(triggerTarget, 15101, 15.0f);
						MaNGOS::CreatureListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(lList, check);
						Cell::VisitGridObjects(triggerTarget, searcher, 15.0f);

						for (std::list<Creature*>::const_iterator itr = lList.begin(); itr != lList.end(); ++itr)
							if ((*itr)->isAlive())
								(*itr)->AddThreat(triggerTarget, float(5000));

						return;
					}
					//                    // Restoration
					//                    case 24379: break;
					//                    // Happy Pet
					//                    case 24716: break;
				case 24780:                             // Dream Fog
					{
						// Note: In 1.12 triggered spell 24781 still exists, need to script dummy effect for this spell then
						// Select an unfriendly enemy in 100y range and attack it
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						ThreatList const& tList = target->getThreatManager().getThreatList();
						for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
						{
							Unit* pUnit = target->GetMap()->GetUnit((*itr)->getUnitGuid());

							if (pUnit && target->getThreatManager().getThreat(pUnit))
								target->getThreatManager().modifyThreatPercent(pUnit, -100);
						}

						if (Unit* pEnemy = target->SelectRandomUnfriendlyTarget(target->getVictim(), 100.0f))
							((Creature*)target)->AI()->AttackStart(pEnemy);

						return;
					}
					//                    // Cannon Prep
					//                    case 24832: break;
				case 24834:                             // Shadow Bolt Whirl
					{
						uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
						uint32 tick = (GetAuraTicks() + 7/*-1*/) % 8;

						// casted in left/right (but triggered spell have wide forward cone)
						float forward = target->GetOrientation();
						if (tick <= 3)
							target->SetOrientation(forward + 0.75f * M_PI_F - tick * M_PI_F / 8);       // Left
						else
							target->SetOrientation(forward - 0.75f * M_PI_F + (8 - tick) * M_PI_F / 8); // Right

						triggerTarget->CastSpell(triggerTarget, spellForTick[tick], true, NULL, this, casterGUID);
						target->SetOrientation(forward);
						return;
					}
					//                    // Stink Trap
					//                    case 24918: break;
					//                    // Agro Drones
					//                    case 25152: break;
				case 25371:                             // Consume
					{
						int32 bpDamage = triggerTarget->GetMaxHealth() * 10 / 100;
						triggerTarget->CastCustomSpell(triggerTarget, 25373, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
						return;
					}
					//                    // Pain Spike
					//                    case 25572: break;
				case 26009:                             // Rotate 360
				case 26136:                             // Rotate -360
					{
						float newAngle = target->GetOrientation();

						if (auraId == 26009)
							newAngle += M_PI_F / 40;
						else
							newAngle -= M_PI_F / 40;

						newAngle = MapManager::NormalizeOrientation(newAngle);

						target->SetFacingTo(newAngle);

						target->CastSpell(target, 26029, true);
						return;
					}
					//                    // Consume
					//                    case 26196: break;
					//                    // Berserk
					//                    case 26615: break;
					//                    // Defile
					//                    case 27177: break;
					//                    // Teleport: IF/UC
					//                    case 27601: break;
					//                    // Five Fat Finger Exploding Heart Technique
					//                    case 27673: break;
					//                    // Nitrous Boost
					//                    case 27746: break;
					//                    // Steam Tank Passive
					//                    case 27747: break;
				case 27808:                             // Frost Blast
					{
						int32 bpDamage = triggerTarget->GetMaxHealth() * 26 / 100;
						triggerTarget->CastCustomSpell(triggerTarget, 29879, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
						return;
					}
					// Detonate Mana
				case 27819:
					{
						// 50% Mana Burn
						int32 bpDamage = (int32)triggerTarget->GetPower(POWER_MANA) * 0.5f;
						triggerTarget->ModifyPower(POWER_MANA, -bpDamage);
						triggerTarget->CastCustomSpell(triggerTarget, 27820, &bpDamage, NULL, NULL, true, NULL, this, triggerTarget->GetObjectGuid());
						return;
					}
					//                    // Controller Timer
					//                    case 28095: break;
					// Stalagg Chain and Feugen Chain
				case 28096:
				case 28111:
					{
						// X-Chain is casted by Tesla to X, so: caster == Tesla, target = X
						Unit* pCaster = GetCaster();
						if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT && !pCaster->IsWithinDistInMap(target, 60.0f))
						{
							pCaster->InterruptNonMeleeSpells(true);
							((Creature*)pCaster)->SetInCombatWithZone();
							// Stalagg Tesla Passive or Feugen Tesla Passive
							pCaster->CastSpell(pCaster, auraId == 28096 ? 28097 : 28109, true, NULL, NULL, target->GetObjectGuid());
						}
						return;
					}
					// Stalagg Tesla Passive and Feugen Tesla Passive
				case 28097:
				case 28109:
					{
						// X-Tesla-Passive is casted by Tesla on Tesla with original caster X, so: caster = X, target = Tesla
						Unit* pCaster = GetCaster();
						if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT)
						{
							if (pCaster->getVictim() && !pCaster->IsWithinDistInMap(target, 60.0f))
							{
								if (Unit* pTarget = ((Creature*)pCaster)->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
									target->CastSpell(pTarget, 28099, false);// Shock
							}
							else
							{
								// "Evade"
								target->RemoveAurasDueToSpell(auraId);
								target->DeleteThreatList();
								target->CombatStop(true);
								// Recast chain (Stalagg Chain or Feugen Chain
								target->CastSpell(pCaster, auraId == 28097 ? 28096 : 28111, false);
							}
						}
						return;
					}
					//                    // Mark of Didier
					//                    case 28114: break;
					//                    // Communique Timer, camp
					//                    case 28346: break;
					//                    // Icebolt
					//                    case 28522: break;
					//                    // Silithyst
					//                    case 29519: break;
				case 29528:                             // Inoculate Nestlewood Owlkin
					// prevent error reports in case ignored player target
					if (triggerTarget->GetTypeId() != TYPEID_UNIT)
						return;
					break;
					//                    // Overload
					//                    case 29768: break;
					//                    // Return Fire
					//                    case 29788: break;
					//                    // Return Fire
					//                    case 29793: break;
					//                    // Return Fire
					//                    case 29794: break;
					//                    // Guardian of Icecrown Passive
					//                    case 29897: break;
				case 29917:                             // Feed Captured Animal
					trigger_spell_id = 29916;
					break;
					//                    // Flame Wreath
					//                    case 29946: break;
					//                    // Flame Wreath
					//                    case 29947: break;
					//                    // Mind Exhaustion Passive
					//                    case 30025: break;
					//                    // Nether Beam - Serenity
					//                    case 30401: break;
				case 30427:                             // Extract Gas
					{
						Unit* caster = GetCaster();
						if (!caster)
							return;
						// move loot to player inventory and despawn target
						if (caster->GetTypeId() == TYPEID_PLAYER &&
							triggerTarget->GetTypeId() == TYPEID_UNIT &&
							((Creature*)triggerTarget)->GetCreatureInfo()->type == CREATURE_TYPE_GAS_CLOUD)
						{
							Player* player = (Player*)caster;
							Creature* creature = (Creature*)triggerTarget;
							// missing lootid has been reported on startup - just return
							if (!creature->GetCreatureInfo()->SkinLootId)
								return;

							player->AutoStoreLoot(creature, creature->GetCreatureInfo()->SkinLootId, LootTemplates_Skinning, true);

							creature->ForcedDespawn();
						}
						return;
					}
				case 30576:                             // Quake
					trigger_spell_id = 30571;
					break;
					//                    // Burning Maul
					//                    case 30598: break;
					//                    // Regeneration
					//                    case 30799:
					//                    case 30800:
					//                    case 30801:
					//                        break;
					//                    // Despawn Self - Smoke cloud
					//                    case 31269: break;
					//                    // Time Rift Periodic
					//                    case 31320: break;
					//                    // Corrupt Medivh
					//                    case 31326: break;
				case 31347:                             // Doom
					{
						target->CastSpell(target, 31350, true);
						target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
						return;
					}
				case 31373:                             // Spellcloth
					{
						// Summon Elemental after create item
						triggerTarget->SummonCreature(17870, 0.0f, 0.0f, 0.0f, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
						return;
					}
					//                    // Bloodmyst Tesla
					//                    case 31611: break;
				case 31944:                             // Doomfire
					{
						int32 damage = m_modifier.m_amount * (float(GetAuraDuration()) + float(m_modifier.periodictime)) / float(GetAuraMaxDuration());
						triggerTarget->CastCustomSpell(triggerTarget, 31969, &damage, NULL, NULL, true, NULL, this, casterGUID);
						return;
					}
					//                    // Teleport Test
					//                    case 32236: break;
				case 32686:				  // Earthquake
					{
						triggerCaster->CastSpell(triggerTarget, 15753, true);
						return;
					}
					//                    // Possess
					//                    case 33401: break;
					//                    // Draw Shadows
					//                    case 33563: break;
					//                    // Murmur's Touch
					//                    case 33711: break;
				case 34229:                             // Flame Quills
					{
						// cast 24 spells 34269-34289, 34314-34316
						for (uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
							triggerTarget->CastSpell(triggerTarget, spell_id, true, NULL, this, casterGUID);
						for (uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
							triggerTarget->CastSpell(triggerTarget, spell_id, true, NULL, this, casterGUID);
						return;
					}
					//                    // Gravity Lapse
					//                    case 34480: break;
					//                    // Tornado
					//                    case 34683: break;
					//                    // Frostbite Rotate
					//                    case 34748: break;
					//                    // Arcane Flurry
					//                    case 34821: break;
					//                    // Interrupt Shutdown
					//                    case 35016: break;
					//                    // Interrupt Shutdown
					//                    case 35176: break;
					//                    // Inferno
					//                    case 35268: break;
					//                    // Salaadin's Tesla
					//                    case 35515: break;
					//                    // Ethereal Channel (Red)
					//                    case 35518: break;
					//                    // Nether Vapor
					//                    case 35879: break;
					//                    // Dark Portal Storm
					//                    case 36018: break;
					//                    // Burning Maul
					//                    case 36056: break;
					//                    // Living Grove Defender Lifespan
					//                    case 36061: break;
					//                    // Professor Dabiri Talks
					//                    case 36064: break;
					//                    // Kael Gaining Power
					//                    case 36091: break;
					//                    // They Must Burn Bomb Aura
					//                    case 36344: break;
					//                    // They Must Burn Bomb Aura (self)
					//                    case 36350: break;
					//                    // Stolen Ravenous Ravager Egg
					//                    case 36401: break;
					//                    // Activated Cannon
					//                    case 36410: break;
					//                    // Stolen Ravenous Ravager Egg
					//                    case 36418: break;
					//                    // Enchanted Weapons
					//                    case 36510: break;
					//                    // Cursed Scarab Periodic
					//                    case 36556: break;
					//                    // Cursed Scarab Despawn Periodic
					//                    case 36561: break;
				case 36573:                             // Vision Guide
					{
						if (GetAuraTicks() == 10 && target->GetTypeId() == TYPEID_PLAYER)
						{
							((Player*)target)->AreaExploredOrEventHappens(10525);
							target->RemoveAurasDueToSpell(36573);
						}

						return;
					}
					//                    // Cannon Charging (platform)
					//                    case 36785: break;
					//                    // Cannon Charging (self)
					//                    case 36860: break;
				case 37027:                             // Remote Toy
					trigger_spell_id = 37029;
					break;
					//                    // Mark of Death
					//                    case 37125: break;
					//                    // Arcane Flurry
					//                    case 37268: break;
				case 37429:                             // Spout (left)
				case 37430:                             // Spout (right)
					{
						float newAngle = target->GetOrientation();

						if (auraId == 37429)
							newAngle += 2 * M_PI_F / 100;
						else
							newAngle -= 2 * M_PI_F / 100;

						newAngle = MapManager::NormalizeOrientation(newAngle);

						target->SetFacingTo(newAngle);

						target->CastSpell(target, 37433, true);
						return;
					}
					//                    // Karazhan - Chess NPC AI, Snapshot timer
					//                    case 37440: break;
					//                    // Karazhan - Chess NPC AI, action timer
					//                    case 37504: break;
					//                    // Karazhan - Chess: Is Square OCCUPIED aura (DND)
					//                    case 39400: break;
					//                    // Banish
					//                    case 37546: break;
					//                    // Shriveling Gaze
					//                    case 37589: break;
					//                    // Fake Aggro Radius (2 yd)
					//                    case 37815: break;
					//                    // Corrupt Medivh
					//                    case 37853: break;
				case 38495:                             // Eye of Grillok
					{
						target->CastSpell(target, 38530, true);
						return;
					}
				case 38554:                             // Absorb Eye of Grillok (Zezzak's Shard)
					{
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						if (Unit* caster = GetCaster())
							caster->CastSpell(caster, 38495, true, NULL, this);
						else
							return;

						Creature* creatureTarget = (Creature*)target;

						creatureTarget->ForcedDespawn();
						return;
					}
					//                    // Magic Sucker Device timer
					//                    case 38672: break;
					//                    // Tomb Guarding Charging
					//                    case 38751: break;
					//                    // Murmur's Touch
					//                    case 38794: break;
				case 39105:                             // Activate Nether-wraith Beacon (31742 Nether-wraith Beacon item)
					{
						float fX, fY, fZ;
						triggerTarget->GetClosePoint(fX, fY, fZ, triggerTarget->GetObjectBoundingRadius(), 20.0f);
						triggerTarget->SummonCreature(22408, fX, fY, fZ, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
						return;
					}
					// Drain World Tree Visual @Kordbc
				case 39140: 
					trigger_spell_id = 39141;
					break;
					//                    // Quest - Dustin's Undead Dragon Visual aura
					//                    case 39259: break;
					//                    // Hellfire - The Exorcism, Jules releases darkness, aura
					//                    case 39306: break;
					//                    // Inferno
					//                    case 39346: break;
					//                    // Enchanted Weapons
					//                    case 39489: break;
					//                    // Shadow Bolt Whirl
					//                    case 39630: break;
					//                    // Shadow Bolt Whirl
					//                    case 39634: break;
					// Shadow Inferno @Kordbc
				case 39645: 
					trigger_spell_id = 39646;
					break;
				case 39857:                             // Tear of Azzinoth Summon Channel - it's not really supposed to do anything,and this only prevents the console spam
					trigger_spell_id = 39856;
					break;
					//                    // Soulgrinder Ritual Visual (Smashed)
					//                    case 39974: break;
					//                    // Simon Game Pre-game timer
					//                    case 40041: break;
					//                    // Knockdown Fel Cannon: The Aggro Check Aura
					//                    case 40113: break;
					//                    // Spirit Lance
					//                    case 40157: break;
				case 40398:                             // Demon Transform 2
					switch (GetAuraTicks())
					{
					case 1:
						if (target->HasAura(40506))
							target->RemoveAurasDueToSpell(40506);
						else
							trigger_spell_id = 40506;
						break;
					case 2:
						trigger_spell_id = 40510;
						break;
					}
					break;
				case 40511:                             // Demon Transform 1
					trigger_spell_id = 40398;
					break;
					//                    // Ancient Flames
					//                    case 40657: break;
					//                    // Ethereal Ring Cannon: Cannon Aura
					//                    case 40734: break;
					//                    // Cage Trap
					//                    case 40760: break;
					//                    // Random Periodic
					//                    case 40867: break;
					//                    // Prismatic Shield
					//                    case 40879: break;
				case 41350:				  // Aura of Desire
					trigger_spell_id = 41352;
					break;
					//                    // Dementia
					//                    case 41404: break;
					//                    // Chaos Form
					//                    case 41629: break;
					//                    // Alert Drums
					//                    case 42177: break;
					//                    // Spout
					//                    case 42581: break;
					//                    // Spout
					//                    case 42582: break;
					//                    // Return to the Spirit Realm
					//                    case 44035: break;
					//                    // Curse of Boundless Agony
					//                    case 45050: break;
					//                    // Earthquake
					//                    case 46240: break;
				case 46736:                             // Personalized Weather
					trigger_spell_id = 46737;
					break;
					//                    // Stay Submerged
					//                    case 46981: break;
					//                    // Dragonblight Ram
					//                    case 47015: break;
					//                    // Party G.R.E.N.A.D.E.
					//                    case 51510: break;
				default:
					break;
				}
				break;
			}
		case SPELLFAMILY_MAGE:
			{
				switch (auraId)
				{
				case 66:                                // Invisibility
					// Here need periodic trigger reducing threat spell (or do it manually)
					return;
				default:
					break;
				}
				break;
			}
			//            case SPELLFAMILY_WARRIOR:
			//            {
			//                switch(auraId)
			//                {
			//                    // Wild Magic
			//                    case 23410: break;
			//                    // Corrupted Totems
			//                    case 23425: break;
			//                    default:
			//                        break;
			//                }
			//                break;
			//            }
			//            case SPELLFAMILY_PRIEST:
			//            {
			//                switch(auraId)
			//                {
			//                    // Blue Beam
			//                    case 32930: break;
			//                    // Fury of the Dreghood Elders
			//                    case 35460: break;
			//                    default:
			//                        break;
			//                }
			//                break;
			//            }
		case SPELLFAMILY_DRUID:
			{
				switch (auraId)
				{
				case 768:                               // Cat Form
					// trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
					return;
				case 22842:                             // Frenzied Regeneration
				case 22895:
				case 22896:
				case 26999:
					{
						int32 LifePerRage = GetModifier()->m_amount;

						int32 lRage = target->GetPower(POWER_RAGE);
						if (lRage > 100)                    // rage stored as rage*10
							lRage = 100;
						target->ModifyPower(POWER_RAGE, -lRage);
						int32 FRTriggerBasePoints = int32(lRage * LifePerRage / 10);
						target->CastCustomSpell(target, 22845, &FRTriggerBasePoints, NULL, NULL, true, NULL, this);
						return;
					}
				default:
					break;
				}
				break;
			}

			//            case SPELLFAMILY_HUNTER:
			//            {
			//                switch(auraId)
			//                {
			//                    // Frost Trap Aura
			//                    case 13810:
			//                        return;
			//                    // Rizzle's Frost Trap
			//                    case 39900:
			//                        return;
			//                    // Tame spells
			//                    case 19597:         // Tame Ice Claw Bear
			//                    case 19676:         // Tame Snow Leopard
			//                    case 19677:         // Tame Large Crag Boar
			//                    case 19678:         // Tame Adult Plainstrider
			//                    case 19679:         // Tame Prairie Stalker
			//                    case 19680:         // Tame Swoop
			//                    case 19681:         // Tame Dire Mottled Boar
			//                    case 19682:         // Tame Surf Crawler
			//                    case 19683:         // Tame Armored Scorpid
			//                    case 19684:         // Tame Webwood Lurker
			//                    case 19685:         // Tame Nightsaber Stalker
			//                    case 19686:         // Tame Strigid Screecher
			//                    case 30100:         // Tame Crazed Dragonhawk
			//                    case 30103:         // Tame Elder Springpaw
			//                    case 30104:         // Tame Mistbat
			//                    case 30647:         // Tame Barbed Crawler
			//                    case 30648:         // Tame Greater Timberstrider
			//                    case 30652:         // Tame Nightstalker
			//                        return;
			//                    default:
			//                        break;
			//                }
			//                break;
			//            }
		case SPELLFAMILY_SHAMAN:
			{
				switch (auraId)
				{
				case 28820:                             // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
					{
						// Need remove self if Lightning Shield not active
						Unit::SpellAuraHolderMap const& auras = triggerTarget->GetSpellAuraHolderMap();
						for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
						{
							SpellEntry const* spell = itr->second->GetSpellProto();
							if (spell->SpellFamilyName == SPELLFAMILY_SHAMAN &&
								(spell->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
								return;
						}
						triggerTarget->RemoveAurasDueToSpell(28820);
						return;
					}
				case 38443:                             // Totemic Mastery (Skyshatter Regalia (Shaman Tier 6) - bonus)
					{
						if (triggerTarget->IsAllTotemSlotsUsed())
							triggerTarget->CastSpell(triggerTarget, 38437, true, NULL, this);
						else
							triggerTarget->RemoveAurasDueToSpell(38437);
						return;
					}
				default:
					break;
				}
				break;
			}
		default:
			break;
		}

		// Reget trigger spell proto
		triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
	}
	else                                                    // initial triggeredSpellInfo != NULL
	{
		// for channeled spell cast applied from aura owner to channel target (persistent aura affects already applied to true target)
		// come periodic casts applied to targets, so need seelct proper caster (ex. 15790)
		if (IsChanneledSpell(GetSpellProto()) && GetSpellProto()->Effect[GetEffIndex()] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
		{
			// interesting 2 cases: periodic aura at caster of channeled spell
			if (target->GetObjectGuid() == casterGUID)
			{
				triggerCaster = target;

				if (WorldObject* channelTarget = target->GetMap()->GetWorldObject(target->GetChannelObjectGuid()))
				{
					if (channelTarget->isType(TYPEMASK_UNIT))
						triggerTarget = (Unit*)channelTarget;
					else
						triggerTargetObject = channelTarget;
				}
			}
			// or periodic aura at caster channel target
			else if (Unit* caster = GetCaster())
			{
				if (target->GetObjectGuid() == caster->GetChannelObjectGuid())
				{
					triggerCaster = caster;
					triggerTarget = target;
				}
			}
		}

		// Spell exist but require custom code
		switch (auraId)
		{
		case 9347:                                      // Mortal Strike
			{
				if (target->GetTypeId() != TYPEID_UNIT)
					return;
				// expected selection current fight target
				triggerTarget = ((Creature*)target)->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0, triggeredSpellInfo);
				if (!triggerTarget)
					return;

				break;
			}
		case 1010:                                      // Curse of Idiocy
			{
				// TODO: spell casted by result in correct way mostly
				// BUT:
				// 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
				//      but must show affect apply like item casting
				// 2) maybe aura must be replace by new with accumulative stat mods instead stacking

				// prevent cast by triggered auras
				if (casterGUID == triggerTarget->GetObjectGuid())
					return;

				// stop triggering after each affected stats lost > 90
				int32 intelectLoss = 0;
				int32 spiritLoss = 0;

				Unit::AuraList const& mModStat = triggerTarget->GetAurasByType(SPELL_AURA_MOD_STAT);
				for (Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
				{
					if ((*i)->GetId() == 1010)
					{
						switch ((*i)->GetModifier()->m_miscvalue)
						{
						case STAT_INTELLECT: intelectLoss += (*i)->GetModifier()->m_amount; break;
						case STAT_SPIRIT:    spiritLoss   += (*i)->GetModifier()->m_amount; break;
						default: break;
						}
					}
				}

				if (intelectLoss <= -90 && spiritLoss <= -90)
					return;

				break;
			}
		case 16191:                                     // Mana Tide
			{
				triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
				return;
			}
		case 33525:                                     // Ground Slam
			triggerTarget->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this, casterGUID);
			return;
		case 38736:                                     // Rod of Purification - for quest 10839 (Veil Skith: Darkstone of Terokk)
			{
				if (Unit* caster = GetCaster())
					caster->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this);
				return;
			}
		case 43149:                                     // Claw Rage
			{
				// Need to provide explicit target for trigger spell target combination
				target->CastSpell(target->getVictim(), trigger_spell_id, true, NULL, this);
				return;
			}
		case 44883:                                     // Encapsulate
			{
				// Self cast spell, hence overwrite caster (only channeled spell where the triggered spell deals dmg to SELF)
				triggerCaster = triggerTarget;
				break;
			}
		}
	}

	// All ok cast by default case
	if (triggeredSpellInfo)
	{
		if (triggerTargetObject)
			triggerCaster->CastSpell(triggerTargetObject->GetPositionX(), triggerTargetObject->GetPositionY(), triggerTargetObject->GetPositionZ(),
			triggeredSpellInfo, true, NULL, this, casterGUID);
		else
			triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, NULL, this, casterGUID);
	}
	else
	{
		if (Unit* caster = GetCaster())
		{
			if (triggerTarget->GetTypeId() != TYPEID_UNIT || !sScriptMgr.OnEffectDummy(caster, GetId(), GetEffIndex(), (Creature*)triggerTarget, ObjectGuid()))
				sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?", GetId(), GetEffIndex());
		}
	}
}

void Aura::TriggerSpellWithValue()
{
	ObjectGuid casterGuid = GetCasterGuid();
	Unit* target = GetTriggerTarget();

	if (!casterGuid || !target)
		return;

	// generic casting code with custom spells and target/caster customs
	uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];
	int32  basepoints0 = GetModifier()->m_amount;

	// Chaotic Charge
	if(trigger_spell_id == 41039)
	{
		if(Aura* holder = target->GetDummyAura(41033))
			basepoints0 = basepoints0 * holder->GetHolder()->GetStackAmount();
		else 
			return;
	}

	target->CastCustomSpell(target, trigger_spell_id, &basepoints0, NULL, NULL, true, NULL, this, casterGuid);
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	// AT APPLY
	if (apply)
	{
		switch (GetSpellProto()->SpellFamilyName)
		{
		case SPELLFAMILY_GENERIC:
			{
				switch (GetId())
				{
				case 1515:                              // Tame beast
					// FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
					if (target->CanHaveThreatList())
						if (Unit* caster = GetCaster())
							target->AddThreat(caster, 10.0f, false, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
					return;
				case 7057:                              // Haunting Spirits
					// expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
					m_isPeriodic = true;
					m_modifier.periodictime = 30 * IN_MILLISECONDS;
					m_periodicTimer = m_modifier.periodictime;
					return;
				case 10255:                             // Stoned
					{
						if (Unit* caster = GetCaster())
						{
							if (caster->GetTypeId() != TYPEID_UNIT)
								return;

							caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
							caster->addUnitState(UNIT_STAT_ROOT);
						}
						return;
					}
				case 13139:                             // net-o-matic
					// root to self part of (root_target->charge->root_self sequence
					if (Unit* caster = GetCaster())
						caster->CastSpell(caster, 13138, true, NULL, this);
					return;
				case 28832:                             // Mark of Korth'azz
				case 28833:                             // Mark of Blaumeux
				case 28834:                             // Mark of Rivendare
				case 28835:                             // Mark of Zeliek
					{
						int32 damage = 0;

						switch (GetStackAmount())
						{
						case 1:
							return;
						case 2: damage =   500; break;
						case 3: damage =  1500; break;
						case 4: damage =  4000; break;
						case 5: damage = 12500; break;
						default:
							damage = 14000 + 1000 * GetStackAmount();
							break;
						}

						if (Unit* caster = GetCaster())
							caster->CastCustomSpell(target, 28836, &damage, NULL, NULL, true, NULL, this);
						return;
					}
				case 31606:                             // Stormcrow Amulet
					{
						CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(17970);

						// we must assume db or script set display id to native at ending flight (if not, target is stuck with this model)
						if (cInfo)
							target->SetDisplayId(Creature::ChooseDisplayId(cInfo));

						return;
					}
				case 32014:
					{
						// Air Burst (Archimonde) Prevent remove fear @Kordbc
						if(target->HasAura(31970))
							target->RemoveAurasDueToSpell(31970);
						return;
					}
				case 33326:                             // Stolen Soul Dispel
					{
						target->RemoveAurasDueToSpell(32346);
						return;
					}
				case 36587:                             // Vision Guide
					{
						target->CastSpell(target, 36573, true, NULL, this);
						return;
					}
				case 36636:								// Doomwalker Temple Attack Visual
					{
						target->CastSpell(target, 46345, true);
						return;
					}
					// Gender spells
				case 38224:                             // Illidari Agent Illusion
				case 37096:                             // Blood Elf Illusion
				case 46354:                             // Blood Elf Illusion
					{
						uint8 gender = target->getGender();
						uint32 spellId;
						switch (GetId())
						{
						case 38224: spellId = (gender == GENDER_MALE ? 38225 : 38227); break;
						case 37096: spellId = (gender == GENDER_MALE ? 37093 : 37095); break;
						case 46354: spellId = (gender == GENDER_MALE ? 46355 : 46356); break;
						default: return;
						}
						target->CastSpell(target, spellId, true, NULL, this);
						return;
					}
				case 39850:                             // Rocket Blast
					if (roll_chance_i(20))              // backfire stun
						target->CastSpell(target, 51581, true, NULL, this);
					return;
				case 43873:                             // Headless Horseman Laugh
					target->PlayDistanceSound(11965);
					return;
				case 46699:                             // Requires No Ammo
					if (target->GetTypeId() == TYPEID_PLAYER)
						// not use ammo and not allow use
							((Player*)target)->RemoveAmmo();
					return;
				case 48025:                             // Headless Horseman's Mount
					Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 51621, 48024, 51617, 48023, 0);
					return;
				}
				break;
			}
		case SPELLFAMILY_WARRIOR:
			{
				switch (GetId())
				{
				case 41099:                             // Battle Stance
					{
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						// Stance Cooldown
						target->CastSpell(target, 41102, true, NULL, this);

						// Battle Aura
						target->CastSpell(target, 41106, true, NULL, this);

						// equipment
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32614);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
						return;
					}
				case 41100:                             // Berserker Stance
					{
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						// Stance Cooldown
						target->CastSpell(target, 41102, true, NULL, this);

						// Berserker Aura
						target->CastSpell(target, 41107, true, NULL, this);

						// equipment
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32614);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
						return;
					}
				case 41101:                             // Defensive Stance
					{
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						// Stance Cooldown
						target->CastSpell(target, 41102, true, NULL, this);

						// Defensive Aura
						target->CastSpell(target, 41105, true, NULL, this);

						// equipment
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32604);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 31467);
						((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
						return;
					}
				}
				break;
			}
		case SPELLFAMILY_SHAMAN:
			{
				// Earth Shield
				if ((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x40000000000)))
				{
					// prevent double apply bonuses
					if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
					{
						if (Unit* caster = GetCaster())
						{
							m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
							m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
						}
					}
					return;
				}
				break;
			}
		case SPELLFAMILY_PALADIN:
			{
				// Bonus 2p T4 Paladin
				if((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x8000000)))
				{
					if(Aura* aura = GetCaster()->GetDummyAura(37182))
					{
						m_modifier.m_amount += aura->GetModifier()->m_amount;
					}
				}
				break;
			}
		}
	}
	// AT REMOVE
	else
	{
		if (IsQuestTameSpell(GetId()) && target->isAlive())
		{
			Unit* caster = GetCaster();
			if (!caster || !caster->isAlive())
				return;

			uint32 finalSpellId = 0;
			switch (GetId())
			{
			case 19548: finalSpellId = 19597; break;
			case 19674: finalSpellId = 19677; break;
			case 19687: finalSpellId = 19676; break;
			case 19688: finalSpellId = 19678; break;
			case 19689: finalSpellId = 19679; break;
			case 19692: finalSpellId = 19680; break;
			case 19693: finalSpellId = 19684; break;
			case 19694: finalSpellId = 19681; break;
			case 19696: finalSpellId = 19682; break;
			case 19697: finalSpellId = 19683; break;
			case 19699: finalSpellId = 19685; break;
			case 19700: finalSpellId = 19686; break;
			case 30646: finalSpellId = 30647; break;
			case 30653: finalSpellId = 30648; break;
			case 30654: finalSpellId = 30652; break;
			case 30099: finalSpellId = 30100; break;
			case 30102: finalSpellId = 30103; break;
			case 30105: finalSpellId = 30104; break;
			}

			if (finalSpellId)
				caster->CastSpell(target, finalSpellId, true, NULL, this);

			return;
		}

		switch (GetId())
		{
		case 10255:                                     // Stoned
			{
				if (Unit* caster = GetCaster())
				{
					if (caster->GetTypeId() != TYPEID_UNIT)
						return;

					// see dummy effect of spell 10254 for removal of flags etc
					caster->CastSpell(caster, 10254, true);
				}
				return;
			}
		case 12479:                                     // Hex of Jammal'an
			target->CastSpell(target, 12480, true, NULL, this);
			return;
		case 12774:                                     // (DND) Belnistrasz Idol Shutdown Visual
			{
				if (m_removeMode == AURA_REMOVE_BY_DEATH)
					return;

				// Idom Rool Camera Shake <- wtf, don't drink while making spellnames?
				if (Unit* caster = GetCaster())
					caster->CastSpell(caster, 12816, true);

				return;
			}
		case 28169:                                     // Mutating Injection
			{
				// Mutagen Explosion
				target->CastSpell(target, 28206, true, NULL, this);
				// Poison Cloud
				target->CastSpell(target, 28240, true, NULL, this);
				return;
			}
		case 32286:                                     // Focus Target Visual
			{
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
					target->CastSpell(target, 32301, true, NULL, this);

				return;
			}
		case 35079:                                     // Misdirection, triggered buff
			{
				if (Unit* pCaster = GetCaster())
					pCaster->RemoveAurasDueToSpell(34477);
				return;
			}
		case 36730:                                     // Flame Strike
			{
				target->CastSpell(target, 36731, true, NULL, this);
				return;
			}
		case 41099:                                     // Battle Stance
			{
				// Battle Aura
				target->RemoveAurasDueToSpell(41106);
				return;
			}
		case 41100:                                     // Berserker Stance
			{
				// Berserker Aura
				target->RemoveAurasDueToSpell(41107);
				return;
			}
		case 41101:                                     // Defensive Stance
			{
				// Defensive Aura
				target->RemoveAurasDueToSpell(41105);
				return;
			}
		case 42385:                                     // Alcaz Survey Aura
			{
				target->CastSpell(target, 42316, true, NULL, this);
				return;
			}
		case 42454:                                     // Captured Totem
			{
				if (m_removeMode == AURA_REMOVE_BY_DEFAULT)
				{
					if (target->getDeathState() != CORPSE)
						return;

					Unit* pCaster = GetCaster();

					if (!pCaster)
						return;

					// Captured Totem Test Credit
					if (Player* pPlayer = pCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
						pPlayer->CastSpell(pPlayer, 42455, true);
				}

				return;
			}
		case 42517:                                     // Beam to Zelfrax
			{
				// expecting target to be a dummy creature
				Creature* pSummon = target->SummonCreature(23864, 0.0f, 0.0f, 0.0f, target->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);

				Unit* pCaster = GetCaster();

				if (pSummon && pCaster)
					pSummon->GetMotionMaster()->MovePoint(0, pCaster->GetPositionX(), pCaster->GetPositionY(), pCaster->GetPositionZ());

				return;
			}
		case 44191:                                     // Flame Strike
			{
				if (target->GetMap()->IsDungeon())
				{
					uint32 spellId = target->GetMap()->IsRegularDifficulty() ? 44190 : 46163;

					target->CastSpell(target, spellId, true, NULL, this);
				}
				return;
			}
		case 45934:                                     // Dark Fiend
			{
				// Kill target if dispelled
				if (m_removeMode == AURA_REMOVE_BY_DISPEL)
					target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
				return;
			}
		case 46308:                                     // Burning Winds
			{
				// casted only at creatures at spawn
				target->CastSpell(target, 47287, true, NULL, this);
				return;
			}
		}
	}

	// AT APPLY & REMOVE

	switch (GetSpellProto()->SpellFamilyName)
	{
	case SPELLFAMILY_GENERIC:
		{
			switch (GetId())
			{
			case 6606:                                  // Self Visual - Sleep Until Cancelled (DND)
				{
					if (apply)
					{
						target->SetStandState(UNIT_STAND_STATE_SLEEP);
						target->addUnitState(UNIT_STAT_ROOT);
					}
					else
					{
						target->clearUnitState(UNIT_STAT_ROOT);
						target->SetStandState(UNIT_STAND_STATE_STAND);
					}

					return;
				}
			case 24658:                                 // Unstable Power
				{
					if (apply)
					{
						Unit* caster = GetCaster();
						if (!caster)
							return;

						caster->CastSpell(target, 24659, true, NULL, NULL, GetCasterGuid());
					}
					else
						target->RemoveAurasDueToSpell(24659);
					return;
				}
			case 24661:                                 // Restless Strength
				{
					if (apply)
					{
						Unit* caster = GetCaster();
						if (!caster)
							return;

						caster->CastSpell(target, 24662, true, NULL, NULL, GetCasterGuid());
					}
					else
						target->RemoveAurasDueToSpell(24662);
					return;
				}
			case 29266:                                 // Permanent Feign Death
			case 31261:                                 // Permanent Feign Death (Root)
			case 37493:                                 // Feign Death
				{
					// Unclear what the difference really is between them.
					// Some has effect1 that makes the difference, however not all.
					// Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
					// For now, just handle all the same way
					if (target->GetTypeId() == TYPEID_UNIT)
						target->SetFeignDeath(apply);

					return;
				}
			case 32216:                                 // Victorious
				if (target->getClass() == CLASS_WARRIOR)
					target->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, apply);
				return;
			case 35356:                                 // Spawn Feign Death
			case 35357:                                 // Spawn Feign Death
				{
					if (target->GetTypeId() == TYPEID_UNIT)
					{
						// Flags not set like it's done in SetFeignDeath()
						// UNIT_DYNFLAG_DEAD does not appear with these spells.
						// All of the spells appear to be present at spawn and not used to feign in combat or similar.
						if (apply)
						{
							target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
							target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

							target->addUnitState(UNIT_STAT_DIED);
						}
						else
						{
							target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
							target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

							target->clearUnitState(UNIT_STAT_DIED);
						}
					}
					return;
				}
			case 40133:                                 // Summon Fire Elemental
				{
					Unit* caster = GetCaster();
					if (!caster)
						return;

					Unit* owner = caster->GetOwner();
					if (owner && owner->GetTypeId() == TYPEID_PLAYER)
					{
						if (apply)
							owner->CastSpell(owner, 8985, true);
						else
							((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
					}
					return;
				}
			case 40132:                                 // Summon Earth Elemental
				{
					Unit* caster = GetCaster();
					if (!caster)
						return;

					Unit* owner = caster->GetOwner();
					if (owner && owner->GetTypeId() == TYPEID_PLAYER)
					{
						if (apply)
							owner->CastSpell(owner, 19704, true);
						else
							((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
					}
					return;
				}
			case 40214:                                 // Dragonmaw Illusion
				{
					if (apply)
					{
						target->CastSpell(target, 40216, true);
						target->CastSpell(target, 42016, true);
					}
					else
					{
						target->RemoveAurasDueToSpell(40216);
						target->RemoveAurasDueToSpell(42016);
					}
					return;
				}
			case 42515:                                 // Jarl Beam
				{
					// aura animate dead (fainted) state for the duration, but we need to animate the death itself (correct way below?)
					if (Unit* pCaster = GetCaster())
						pCaster->ApplyModFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH, apply);

					// Beam to Zelfrax at remove
					if (!apply)
						target->CastSpell(target, 42517, true);
					return;
				}
			case 42583:                                 // Claw Rage
				{
					Unit* caster = GetCaster();
					if (!caster || target->GetTypeId() != TYPEID_PLAYER)
						return;

					if (apply)
						caster->FixateTarget(target);
					else if (target->GetObjectGuid() == caster->GetFixateTargetGuid())
						caster->FixateTarget(NULL);

					return;
				}
			case 27978:
			case 40131:
				if (apply)
					target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
				else
					target->m_AuraFlags &= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
				return;
			}
			break;
		}
	case SPELLFAMILY_MAGE:
		{
			// Hypothermia
			if (GetId() == 41425)
			{
				target->ModifyAuraState(AURA_STATE_HYPOTHERMIA, apply);
				return;
			}
			break;
		}
	case SPELLFAMILY_DRUID:
		{
			switch (GetId())
			{
			case 34246:                                 // Idol of the Emerald Queen
				{
					if (target->GetTypeId() != TYPEID_PLAYER)
						return;

					if (apply)
						// dummy not have proper effectclassmask
							m_spellmod  = new SpellModifier(SPELLMOD_DOT, SPELLMOD_FLAT, m_modifier.m_amount / 7, GetId(), UI64LIT(0x001000000000));

					((Player*)target)->AddSpellMod(m_spellmod, apply);
					return;
				}
			}

			// Lifebloom
			if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x1000000000))
			{
				if (apply)
				{
					if (Unit* caster = GetCaster())
					{
						// prevent double apply bonuses
						if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
						{
							// Lifebloom ignore stack amount
							m_modifier.m_amount /= GetStackAmount();
							m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
							m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
						}
					}
				}
				else
				{
					// Final heal on duration end
					if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
						return;

					// final heal
					if (target->IsInWorld() && GetStackAmount() > 0)
					{
						// Lifebloom dummy store single stack amount always
						int32 amount = m_modifier.m_amount;
						target->CastCustomSpell(target, 33778, &amount, NULL, NULL, true, NULL, this, GetCasterGuid());
					}
				}
				return;
			}

			// Predatory Strikes
			if (target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
			{
				((Player*)target)->UpdateAttackPowerAndDamage();
				return;
			}
			break;
		}
	case SPELLFAMILY_ROGUE:
		break;
	case SPELLFAMILY_HUNTER:
		{
			switch (GetId())
			{
				// Improved Aspect of the Viper
			case 38390:
				{
					if (target->GetTypeId() == TYPEID_PLAYER)
					{
						if (apply)
							// + effect value for Aspect of the Viper
								m_spellmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_FLAT, m_modifier.m_amount, GetId(), UI64LIT(0x4000000000000));

						((Player*)target)->AddSpellMod(m_spellmod, apply);
					}
					return;
				}
			}
			break;
		}
	case SPELLFAMILY_SHAMAN:
		{
			switch (GetId())
			{
			case 6495:                                  // Sentry Totem
				{
					if (target->GetTypeId() != TYPEID_PLAYER)
						return;

					Totem* totem = target->GetTotem(TOTEM_SLOT_AIR);

					if (totem && apply)
						((Player*)target)->GetCamera().SetView(totem);
					else
						((Player*)target)->GetCamera().ResetView();

					return;
				}
			}
			// Improved Weapon Totems
			if (GetSpellProto()->SpellIconID == 57 && target->GetTypeId() == TYPEID_PLAYER)
			{
				if (apply)
				{
					switch (m_effIndex)
					{
					case 0:
						// Windfury Totem
						m_spellmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_PCT, m_modifier.m_amount, GetId(), UI64LIT(0x00200000000));
						break;
					case 1:
						// Flametongue Totem
						m_spellmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_PCT, m_modifier.m_amount, GetId(), UI64LIT(0x00400000000));
						break;
					default: return;
					}
				}

				((Player*)target)->AddSpellMod(m_spellmod, apply);
				return;
			}
			break;
		}
	}

	// pet auras
	if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId()))
	{
		if (apply)
			target->AddPetAura(petSpell);
		else
			target->RemovePetAura(petSpell);
		return;
	}

	if (target->GetTypeId() == TYPEID_PLAYER)
	{
		SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
		if (saBounds.first != saBounds.second)
		{
			uint32 zone, area;
			target->GetZoneAndAreaId(zone, area);

			for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
				itr->second->ApplyOrRemoveSpellIfCan((Player*)target, zone, area, false);
		}
	}

	// script has to "handle with care", only use where data are not ok to use in the above code.
	if (target->GetTypeId() == TYPEID_UNIT)
		sScriptMgr.OnAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (apply)
	{
		CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
		if (!ci)
		{
			sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
			return;
		}

		uint32 display_id = Creature::ChooseDisplayId(ci);
		CreatureModelInfo const* minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
		if (minfo)
			display_id = minfo->modelid;

		target->Mount(display_id, GetId());
	}
	else
	{
		target->Unmount(true);
	}
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	GetTarget()->SetWaterWalk(apply);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;
	Unit* target = GetTarget();
	WorldPacket data;
	if (apply)
		data.Initialize(SMSG_MOVE_FEATHER_FALL, 8 + 4);
	else
		data.Initialize(SMSG_MOVE_NORMAL_FALL, 8 + 4);
	data << target->GetPackGUID();
	data << uint32(0);
	target->SendMessageToSet(&data, true);

	// start fall from current height
	if (!apply && target->GetTypeId() == TYPEID_PLAYER)
		((Player*)target)->SetFallInformation(0, target->GetPositionZ());
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	WorldPacket data;
	if (apply)
		data.Initialize(SMSG_MOVE_SET_HOVER, 8 + 4);
	else
		data.Initialize(SMSG_MOVE_UNSET_HOVER, 8 + 4);
	data << GetTarget()->GetPackGUID();
	data << uint32(0);
	GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
	// update timers in client
	if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
		((Player*)GetTarget())->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
	if (!Real)
		return;

	ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

	SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
	if (!ssEntry)
	{
		sLog.outError("Unknown shapeshift form %u in spell %u", form, GetId());
		return;
	}

	uint32 modelid = 0;
	Powers PowerType = POWER_MANA;
	Unit* target = GetTarget();

	if (ssEntry->modelID_A)
	{
		// i will asume that creatures will always take the defined model from the dbc
		// since no field in creature_templates describes wether an alliance or
		// horde modelid should be used at shapeshifting
		if (target->GetTypeId() != TYPEID_PLAYER)
			modelid = ssEntry->modelID_A;
		else
		{
			// players are a bit different since the dbc has seldomly an horde modelid
			if (Player::TeamForRace(target->getRace()) == HORDE)
			{
				// get model for race ( in 2.2.4 no horde models in dbc field, only 0 in it
				modelid = sObjectMgr.GetModelForRace(ssEntry->modelID_A, target->getRaceMask());
			}

			// nothing found in above, so use default
			if (!modelid)
				modelid = ssEntry->modelID_A;
		}
	}

	// remove polymorph before changing display id to keep new display id
	switch (form)
	{
	case FORM_CAT:
	case FORM_TREE:
	case FORM_TRAVEL:
	case FORM_AQUA:
	case FORM_BEAR:
	case FORM_DIREBEAR:
	case FORM_FLIGHT_EPIC:
	case FORM_FLIGHT:
	case FORM_MOONKIN:
		{
			// remove movement affects
			target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT, GetHolder());
			Unit::AuraList const& slowingAuras = target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
			for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
			{
				SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

				uint32 aurMechMask = GetAllSpellMechanicMask(aurSpellInfo);

				// If spell that caused this aura has Croud Control or Daze effect
				if ((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
					// some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
						(aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
						(aurMechMask & (1 << (MECHANIC_SNARE - 1))) == 0))
				{
					++iter;
					continue;
				}

				// All OK, remove aura now
				target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
				iter = slowingAuras.begin();
			}

			// and polymorphic affects
			if (target->IsPolymorphed())
				target->RemoveAurasDueToSpell(target->getTransForm());

			break;
		}
	default:
		break;
	}

	if (apply)
	{
		// remove other shapeshift before applying a new one
		target->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT, GetHolder());

		if (modelid > 0)
			target->SetDisplayId(modelid);

		// now only powertype must be set
		switch (form)
		{
		case FORM_CAT:
			PowerType = POWER_ENERGY;
			break;
		case FORM_BEAR:
		case FORM_DIREBEAR:
		case FORM_BATTLESTANCE:
		case FORM_BERSERKERSTANCE:
		case FORM_DEFENSIVESTANCE:
			PowerType = POWER_RAGE;
			break;
		default:
			break;
		}

		if (PowerType != POWER_MANA)
		{
			// reset power to default values only at power change
			if (target->getPowerType() != PowerType)
				target->setPowerType(PowerType);

			switch (form)
			{
			case FORM_CAT:
			case FORM_BEAR:
			case FORM_DIREBEAR:
				{
					// get furor proc chance
					int32 furorChance = 0;
					Unit::AuraList const& mDummy = target->GetAurasByType(SPELL_AURA_DUMMY);
					for (Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
					{
						if ((*i)->GetSpellProto()->SpellIconID == 238)
						{
							furorChance = (*i)->GetModifier()->m_amount;
							break;
						}
					}

					if (m_modifier.m_miscvalue == FORM_CAT)
					{
						target->SetPower(POWER_ENERGY, 0);
						if (irand(1, 100) <= furorChance)
							target->CastSpell(target, 17099, true, NULL, this);
					}
					else
					{
						target->SetPower(POWER_RAGE, 0);
						if (irand(1, 100) <= furorChance)
							target->CastSpell(target, 17057, true, NULL, this);
					}
					break;
				}
			case FORM_BATTLESTANCE:
			case FORM_DEFENSIVESTANCE:
			case FORM_BERSERKERSTANCE:
				{
					uint32 Rage_val = 0;
					// Stance mastery + Tactical mastery (both passive, and last have aura only in defense stance, but need apply at any stance switch)
					if (target->GetTypeId() == TYPEID_PLAYER)
					{
						PlayerSpellMap const& sp_list = ((Player*)target)->GetSpellMap();
						for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
						{
							if (itr->second.state == PLAYERSPELL_REMOVED)
								continue;

							SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
							if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR && spellInfo->SpellIconID == 139)
								Rage_val += target->CalculateSpellDamage(target, spellInfo, EFFECT_INDEX_0) * 10;
						}
					}

					if (target->GetPower(POWER_RAGE) > Rage_val)
						target->SetPower(POWER_RAGE, Rage_val);
					break;
				}
			default:
				break;
			}
		}

		target->SetShapeshiftForm(form);

		// a form can give the player a new castbar with some spells.. this is a clientside process..
		// serverside just needs to register the new spells so that player isn't kicked as cheater
		if (target->GetTypeId() == TYPEID_PLAYER)
			for (uint32 i = 0; i < 8; ++i)
				if (ssEntry->spellId[i])
					((Player*)target)->addSpell(ssEntry->spellId[i], true, false, false, false);
	}
	else
	{
		if (modelid > 0)
			target->SetDisplayId(target->GetNativeDisplayId());

		if (target->getClass() == CLASS_DRUID)
			target->setPowerType(POWER_MANA);

		target->SetShapeshiftForm(FORM_NONE);

		switch (form)
		{
			// Nordrassil Harness - bonus
		case FORM_BEAR:
		case FORM_DIREBEAR:
		case FORM_CAT:
			if (Aura* dummy = target->GetDummyAura(37315))
				target->CastSpell(target, 37316, true, NULL, dummy);
			break;
			// Nordrassil Regalia - bonus
		case FORM_MOONKIN:
			if (Aura* dummy = target->GetDummyAura(37324))
				target->CastSpell(target, 37325, true, NULL, dummy);
			break;
		default:
			break;
		}

		// look at the comment in apply-part
		if (target->GetTypeId() == TYPEID_PLAYER)
			for (uint32 i = 0; i < 8; ++i)
				if (ssEntry->spellId[i])
					((Player*)target)->removeSpell(ssEntry->spellId[i], false, false, false);
	}

	// adding/removing linked auras
	// add/remove the shapeshift aura's boosts
	HandleShapeshiftBoosts(apply);

	if (target->GetTypeId() == TYPEID_PLAYER)
		((Player*)target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
	Unit* target = GetTarget();
	if (apply)
	{
		// special case (spell specific functionality)
		if (m_modifier.m_miscvalue == 0)
		{
			switch (GetId())
			{
			case 16739:                                 // Orb of Deception
				{
					uint32 orb_model = target->GetNativeDisplayId();
					switch (orb_model)
					{
						// Troll Female
					case 1479: target->SetDisplayId(10134); break;
						// Troll Male
					case 1478: target->SetDisplayId(10135); break;
						// Tauren Male
					case 59:   target->SetDisplayId(10136); break;
						// Human Male
					case 49:   target->SetDisplayId(10137); break;
						// Human Female
					case 50:   target->SetDisplayId(10138); break;
						// Orc Male
					case 51:   target->SetDisplayId(10139); break;
						// Orc Female
					case 52:   target->SetDisplayId(10140); break;
						// Dwarf Male
					case 53:   target->SetDisplayId(10141); break;
						// Dwarf Female
					case 54:   target->SetDisplayId(10142); break;
						// NightElf Male
					case 55:   target->SetDisplayId(10143); break;
						// NightElf Female
					case 56:   target->SetDisplayId(10144); break;
						// Undead Female
					case 58:   target->SetDisplayId(10145); break;
						// Undead Male
					case 57:   target->SetDisplayId(10146); break;
						// Tauren Female
					case 60:   target->SetDisplayId(10147); break;
						// Gnome Male
					case 1563: target->SetDisplayId(10148); break;
						// Gnome Female
					case 1564: target->SetDisplayId(10149); break;
						// BloodElf Female
					case 15475: target->SetDisplayId(17830); break;
						// BloodElf Male
					case 15476: target->SetDisplayId(17829); break;
						// Dranei Female
					case 16126: target->SetDisplayId(17828); break;
						// Dranei Male
					case 16125: target->SetDisplayId(17827); break;
					default: break;
					}
					break;
				}
			case 42365:                                 // Murloc costume
				target->SetDisplayId(21723);
				break;
				// case 44186:                          // Gossip NPC Appearance - All, Brewfest
				// break;
				// case 48305:                          // Gossip NPC Appearance - All, Spirit of Competition
				// break;
			case 50517:                                 // Dread Corsair
			case 51926:                                 // Corsair Costume
				{
					// expected for players
					uint32 race = target->getRace();

					switch (race)
					{
					case RACE_HUMAN:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25037 : 25048);
						break;
					case RACE_ORC:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25039 : 25050);
						break;
					case RACE_DWARF:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25034 : 25045);
						break;
					case RACE_NIGHTELF:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25038 : 25049);
						break;
					case RACE_UNDEAD:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25042 : 25053);
						break;
					case RACE_TAUREN:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25040 : 25051);
						break;
					case RACE_GNOME:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25035 : 25046);
						break;
					case RACE_TROLL:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25041 : 25052);
						break;
					case RACE_GOBLIN:                   // not really player race (3.x), but model exist
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25036 : 25047);
						break;
					case RACE_BLOODELF:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25032 : 25043);
						break;
					case RACE_DRAENEI:
						target->SetDisplayId(target->getGender() == GENDER_MALE ? 25033 : 25044);
						break;
					}

					break;
				}
				// case 50531:                              // Gossip NPC Appearance - All, Pirate Day
				// break;
				// case 51010:                              // Dire Brew
				// break;
			default:
				sLog.outError("Aura::HandleAuraTransform, spell %u does not have creature entry defined, need custom defined model.", GetId());
				break;
			}
		}
		else                                                // m_modifier.m_miscvalue != 0
		{
			uint32 model_id;

			CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
			if (!ci)
			{
				model_id = 16358;                           // pig pink ^_^
				sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", m_modifier.m_miscvalue, GetId());
			}
			else
				model_id = Creature::ChooseDisplayId(ci);   // Will use the default model here

			target->SetDisplayId(model_id);

			// creature case, need to update equipment if additional provided
			if (ci && target->GetTypeId() == TYPEID_UNIT)
				((Creature*)target)->LoadEquipment(ci->equipmentId, false);

			// Dragonmaw Illusion (set mount model also)
			if (GetId() == 42016 && target->GetMountID() && !target->GetAurasByType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED).empty())
				target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
		}

		// update active transform spell only not set or not overwriting negative by positive case
		if (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm()))
			target->setTransForm(GetId());

		// polymorph case
		if (Real && target->GetTypeId() == TYPEID_PLAYER && target->IsPolymorphed())
		{
			// for players, start regeneration after 1s (in polymorph fast regeneration case)
			// only if caster is Player (after patch 2.4.2)
			if (GetCasterGuid().IsPlayer())
				((Player*)target)->setRegenTimer(1 * IN_MILLISECONDS);

			// dismount polymorphed target (after patch 2.4.2)
			if (target->IsMounted())
				target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED, GetHolder());
		}
	}
	else                                                    // !apply
	{
		// ApplyModifier(true) will reapply it if need
		target->setTransForm(0);
		target->SetDisplayId(target->GetNativeDisplayId());

		// apply default equipment for creature case
		if (target->GetTypeId() == TYPEID_UNIT)
			((Creature*)target)->LoadEquipment(((Creature*)target)->GetCreatureInfo()->equipmentId, true);

		// re-apply some from still active with preference negative cases
		Unit::AuraList const& otherTransforms = target->GetAurasByType(SPELL_AURA_TRANSFORM);
		if (!otherTransforms.empty())
		{
			// look for other transform auras
			Aura* handledAura = *otherTransforms.begin();
			for (Unit::AuraList::const_iterator i = otherTransforms.begin(); i != otherTransforms.end(); ++i)
			{
				// negative auras are preferred
				if (!IsPositiveSpell((*i)->GetSpellProto()->Id))
				{
					handledAura = *i;
					break;
				}
			}
			handledAura->ApplyModifier(true);
		}

		// Dragonmaw Illusion (restore mount model)
		if (GetId() == 42016 && target->GetMountID() == 16314)
		{
			if (!target->GetAurasByType(SPELL_AURA_MOUNTED).empty())
			{
				uint32 cr_id = target->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetModifier()->m_miscvalue;
				if (CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(cr_id))
				{
					uint32 display_id = Creature::ChooseDisplayId(ci);
					CreatureModelInfo const* minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
					if (minfo)
						display_id = minfo->modelid;

					target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, display_id);
				}
			}
		}
	}
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (!Real)
		return;

	Player* player = (Player*)GetTarget();

	uint32 faction_id = m_modifier.m_miscvalue;
	ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

	player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
	player->GetReputationMgr().SendForceReactions();

	// stop fighting if at apply forced rank friendly or at remove real rank friendly
	if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
		player->StopAttackFaction(faction_id);
}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	uint32 prot = GetSpellProto()->EffectMiscValue[m_effIndex];
	int32 points = GetModifier()->m_amount;

	((Player*)GetTarget())->ModifySkillBonus(prot, (apply ? points : -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
	if (prot == SKILL_DEFENSE)
		((Player*)GetTarget())->UpdateDefenseBonusesMod();
}

void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
	if (Real && !apply)
	{
		if (m_removeMode != AURA_REMOVE_BY_DEATH)
			return;
		// Item amount
		if (m_modifier.m_amount <= 0)
			return;

		SpellEntry const* spellInfo = GetSpellProto();
		if (spellInfo->EffectItemType[m_effIndex] == 0)
			return;

		Unit* victim = GetTarget();
		Unit* caster = GetCaster();
		if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
			return;

		// Soul Shard (target req.)
		if (spellInfo->EffectItemType[m_effIndex] == 6265)
		{
			// Only from non-grey units
			if (!((Player*)caster)->isHonorOrXPTarget(victim) ||
				(victim->GetTypeId() == TYPEID_UNIT && !((Player*)caster)->isAllowedToLoot((Creature*)victim)))
				return;
		}

		// Adding items
		uint32 noSpaceForCount = 0;
		uint32 count = m_modifier.m_amount;

		ItemPosCountVec dest;
		InventoryResult msg = ((Player*)caster)->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
		if (msg != EQUIP_ERR_OK)
		{
			count -= noSpaceForCount;
			((Player*)caster)->SendEquipError(msg, NULL, NULL, spellInfo->EffectItemType[m_effIndex]);
			if (count == 0)
				return;
		}

		Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
		((Player*)caster)->SendNewItem(newitem, count, true, true);
	}
}

void Aura::HandleModHealthRegenPCT(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();

	if(GetId() == 41292 && target && target->GetTypeId() == TYPEID_PLAYER)
	{
		if(apply)
			target->CastSpell(target, 42017, true);// Proc Suffering aura ROS
		else
			target->RemoveAurasDueToSpell(42017);// Remove aura suffering ROS
	}
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
	Unit* caster = GetCaster();
	if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Camera& camera = ((Player*)caster)->GetCamera();
	if (apply)
		camera.SetView(GetTarget());
	else
		camera.ResetView();
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
	Unit* caster = GetCaster();
	if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Camera& camera = ((Player*)caster)->GetCamera();
	if (apply)
		camera.SetView(GetTarget());
	else
		camera.ResetView();
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (apply)
		GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

	if (apply)
		GetTarget()->SetFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
	else
		GetTarget()->RemoveFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (apply)
		GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

	if (apply)
		GetTarget()->SetFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
	else
		GetTarget()->RemoveFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (apply)
		GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

	GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
	GetTarget()->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
	GetTarget()->UpdateModelData();
}

void Aura::HandleModPossess(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	// not possess yourself
	if (GetCasterGuid() == target->GetObjectGuid())
		return;

	Unit* caster = GetCaster();
	if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* p_caster = (Player*)caster;
	Camera& camera = p_caster->GetCamera();

	if (apply)
	{
		target->addUnitState(UNIT_STAT_CONTROLLED);

		target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
		target->SetCharmerGuid(p_caster->GetObjectGuid());
		target->setFaction(p_caster->getFaction());

		// target should became visible at SetView call(if not visible before):
		// otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
		camera.SetView(target);

		p_caster->SetCharm(target);
		p_caster->SetClientControl(target, 1);
		p_caster->SetMover(target);

		target->CombatStop(true);
		target->DeleteThreatList();
		target->getHostileRefManager().deleteReferences();

		if (CharmInfo* charmInfo = target->InitCharmInfo(target))
		{
			charmInfo->InitPossessCreateSpells();
			charmInfo->SetReactState(REACT_PASSIVE);
			charmInfo->SetCommandState(COMMAND_STAY);
		}

		p_caster->PossessSpellInitialize();

		if (target->GetTypeId() == TYPEID_UNIT)
		{
			((Creature*)target)->AIM_Initialize();
		}
		else if (target->GetTypeId() == TYPEID_PLAYER)
		{
			((Player*)target)->SetClientControl(target, 0);
		}
	}
	else
	{
		p_caster->SetCharm(NULL);

		p_caster->SetClientControl(target, 0);
		p_caster->SetMover(NULL);

		// there is a possibility that target became invisible for client\p_caster at ResetView call:
		// it must be called after movement control unapplying, not before! the reason is same as at aura applying
		camera.ResetView();

		p_caster->RemovePetActionBar();

		// on delete only do caster related effects
		if (m_removeMode == AURA_REMOVE_BY_DELETE)
			return;

		target->clearUnitState(UNIT_STAT_CONTROLLED);

		target->CombatStop(true);
		target->DeleteThreatList();
		target->getHostileRefManager().deleteReferences();

		target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

		target->SetCharmerGuid(ObjectGuid());

		if (target->GetTypeId() == TYPEID_PLAYER)
		{
			((Player*)target)->setFactionForRace(target->getRace());
			((Player*)target)->SetClientControl(target, 1);
		}
		else if (target->GetTypeId() == TYPEID_UNIT)
		{
			CreatureInfo const* cinfo = ((Creature*)target)->GetCreatureInfo();
			target->setFaction(cinfo->faction_A);
		}

		if (target->GetTypeId() == TYPEID_UNIT)
		{
			((Creature*)target)->AIM_Initialize();
			target->AttackedBy(caster);
		}

		// Shadow Of death insta kill (Teron)
		if(GetId() == 40268)
		{
			p_caster->DealDamage(p_caster, p_caster->GetMaxHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
			p_caster->CastSpell(target, 41626, true, NULL, this);
		}
	}
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* caster = GetCaster();
	if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
		return;

	Unit* target = GetTarget();
	if (target->GetTypeId() != TYPEID_UNIT || !((Creature*)target)->IsPet())
		return;

	Pet* pet = (Pet*)target;

	Player* p_caster = (Player*)caster;
	Camera& camera = p_caster->GetCamera();

	if (apply)
	{
		pet->addUnitState(UNIT_STAT_CONTROLLED);

		// target should became visible at SetView call(if not visible before):
		// otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
		camera.SetView(pet);

		p_caster->SetCharm(pet);
		p_caster->SetClientControl(pet, 1);
		((Player*)caster)->SetMover(pet);

		pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

		pet->StopMoving();
		pet->GetMotionMaster()->Clear(false);
		pet->GetMotionMaster()->MoveIdle();
	}
	else
	{
		p_caster->SetCharm(NULL);
		p_caster->SetClientControl(pet, 0);
		p_caster->SetMover(NULL);

		// there is a possibility that target became invisible for client\p_caster at ResetView call:
		// it must be called after movement control unapplying, not before! the reason is same as at aura applying
		camera.ResetView();

		// on delete only do caster related effects
		if (m_removeMode == AURA_REMOVE_BY_DELETE)
			return;

		pet->clearUnitState(UNIT_STAT_CONTROLLED);

		pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

		pet->AttackStop();

		// out of range pet dismissed
		if (!pet->IsWithinDistInMap(p_caster, pet->GetMap()->GetVisibilityDistance()))
		{
			p_caster->RemovePet(PET_SAVE_REAGENTS);
		}
		else
		{
			pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
		}
	}
}

void Aura::HandleModCharm(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	// not charm yourself
	if (GetCasterGuid() == target->GetObjectGuid())
		return;

	Unit* caster = GetCaster();
	if (!caster)
		return;

	if (apply)
	{
		// is it really need after spell check checks?
		target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM, GetHolder());
		target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS, GetHolder());

		target->SetCharmerGuid(GetCasterGuid());
		target->setFaction(caster->getFaction());
		target->CastStop(target == caster ? GetId() : 0);
		caster->SetCharm(target);

		target->CombatStop(true);
		target->DeleteThreatList();
		target->getHostileRefManager().deleteReferences();

		if (target->GetTypeId() == TYPEID_UNIT)
		{
			((Creature*)target)->AIM_Initialize();
			CharmInfo* charmInfo = target->InitCharmInfo(target);
			charmInfo->InitCharmCreateSpells();
			charmInfo->SetReactState(REACT_DEFENSIVE);

			if (caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
			{
				CreatureInfo const* cinfo = ((Creature*)target)->GetCreatureInfo();
				if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
				{
					// creature with pet number expected have class set
					if (target->GetByteValue(UNIT_FIELD_BYTES_0, 1) == 0)
					{
						if (cinfo->unit_class == 0)
							sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.", cinfo->Entry);
						else
							sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.", cinfo->Entry, cinfo->unit_class);

						target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
					}

					// just to enable stat window
					charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
					// if charmed two demons the same session, the 2nd gets the 1st one's name
					target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
				}
			}
		}

		if (caster->GetTypeId() == TYPEID_PLAYER)
			((Player*)caster)->CharmSpellInitialize();
	}
	else
	{
		target->SetCharmerGuid(ObjectGuid());

		if (target->GetTypeId() == TYPEID_PLAYER)
			((Player*)target)->setFactionForRace(target->getRace());
		else
		{
			CreatureInfo const* cinfo = ((Creature*)target)->GetCreatureInfo();

			// restore faction
			if (((Creature*)target)->IsPet())
			{
				if (Unit* owner = target->GetOwner())
					target->setFaction(owner->getFaction());
				else if (cinfo)
					target->setFaction(cinfo->faction_A);
			}
			else if (cinfo)                             // normal creature
				target->setFaction(cinfo->faction_A);

			// restore UNIT_FIELD_BYTES_0
			if (cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
			{
				// DB must have proper class set in field at loading, not req. restore, including workaround case at apply
				// m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

				if (target->GetCharmInfo())
					target->GetCharmInfo()->SetPetNumber(0, true);
				else
					sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", target->GetGUIDLow(), target->GetTypeId());
			}
		}

		caster->SetCharm(NULL);

		if (caster->GetTypeId() == TYPEID_PLAYER)
			((Player*)caster)->RemovePetActionBar();

		target->CombatStop(true);
		target->DeleteThreatList();
		target->getHostileRefManager().deleteReferences();

		if (target->GetTypeId() == TYPEID_UNIT)
		{
			((Creature*)target)->AIM_Initialize();
			target->AttackedBy(caster);
		}
	}
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
	if (!Real)
		return;

	GetTarget()->SetConfused(apply, GetCasterGuid(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
	if (!Real)
		return;

	GetTarget()->SetFeared(apply, GetCasterGuid(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
	if (!Real)
		return;

	GetTarget()->SetFeignDeath(apply, GetCasterGuid(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (!apply && target->HasAuraType(GetModifier()->m_auraname))
		return;

	target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, apply);

	if (target->GetTypeId() != TYPEID_PLAYER)
		return;

	// main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
	if (target->IsInFeralForm())
		return;

	if (apply)
		target->SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
	else
		((Player*)target)->SetRegularAttackTime();

	target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (apply)
	{
		// Frost stun aura -> freeze/unfreeze target
		if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
			target->ModifyAuraState(AURA_STATE_FROZEN, apply);

		target->addUnitState(UNIT_STAT_STUNNED);
		target->SetTargetGuid(ObjectGuid());

		target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
		target->CastStop(target->GetObjectGuid() == GetCasterGuid() ? GetId() : 0);

		// Creature specific
		if (target->GetTypeId() != TYPEID_PLAYER)
			target->StopMoving();
		else
		{
			((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
			target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
			target->SetRoot(true);
		}

		// Summon the Naj'entus Spine GameObject on target if spell is Impaling Spine
		if (GetId() == 39837)
		{
			GameObject* pObj = new GameObject;
			if (pObj->Create(target->GetMap()->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), 185584, target->GetMap(),
				target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation()))
			{
				pObj->SetRespawnTime(GetAuraDuration() / IN_MILLISECONDS);
				pObj->SetSpellId(GetId());
				target->GetMap()->Add(pObj);
				pObj->SetOwnerGuid(ObjectGuid());
				pObj->SummonLinkedTrapIfAny();
			}
			else
				delete pObj;
		}
	}
	else
	{
		// Frost stun aura -> freeze/unfreeze target
		if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
		{
			bool found_another = false;
			for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
			{
				Unit::AuraList const& auras = target->GetAurasByType(*itr);
				for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
				{
					if (GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
					{
						found_another = true;
						break;
					}
				}
				if (found_another)
					break;
			}

			if (!found_another)
				target->ModifyAuraState(AURA_STATE_FROZEN, apply);
		}

		// Real remove called after current aura remove from lists, check if other similar auras active
		if (target->HasAuraType(SPELL_AURA_MOD_STUN))
			return;

		target->clearUnitState(UNIT_STAT_STUNNED);
		target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

		if (!target->hasUnitState(UNIT_STAT_ROOT))        // prevent allow move if have also root effect
		{
			if (target->getVictim() && target->isAlive())
				target->SetTargetGuid(target->getVictim()->GetObjectGuid());

			target->SetRoot(false);
		}

		// Wyvern Sting
		if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000100000000000))
		{
			Unit* caster = GetCaster();
			if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
				return;

			uint32 spell_id = 0;

			switch (GetId())
			{
			case 19386: spell_id = 24131; break;
			case 24132: spell_id = 24134; break;
			case 24133: spell_id = 24135; break;
			case 27068: spell_id = 27069; break;
			default:
				sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?", GetId());
				return;
			}

			SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

			if (!spellInfo)
				return;

			caster->CastSpell(target, spellInfo, true, NULL, this);
			return;
		}
	}
}

void Aura::HandleModStealth(bool apply, bool Real)
{
	Unit* target = GetTarget();

	if (apply)
	{
		// drop flag at stealth in bg
		target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

		// only at real aura add
		if (Real)
		{
			target->SetStandFlags(UNIT_STAND_FLAGS_CREEP);

			if (target->GetTypeId() == TYPEID_PLAYER)
				target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

			// apply only if not in GM invisibility (and overwrite invisibility state)
			if (target->GetVisibility() != VISIBILITY_OFF)
			{
				target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
				target->SetVisibility(VISIBILITY_GROUP_STEALTH);
			}

			// for RACE_NIGHTELF stealth
			if (target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
				target->CastSpell(target, 21009, true, NULL, this);

			// apply full stealth period bonuses only at first stealth aura in stack
			if (target->GetAurasByType(SPELL_AURA_MOD_STEALTH).size() <= 1)
			{
				Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
				for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
				{
					// Master of Subtlety
					if ((*i)->GetSpellProto()->SpellIconID == 2114)
					{
						target->RemoveAurasDueToSpell(31666);
						int32 bp = (*i)->GetModifier()->m_amount;
						target->CastCustomSpell(target, 31665, &bp, NULL, NULL, true);
						break;
					}
				}
			}
		}
	}
	else
	{
		// for RACE_NIGHTELF stealth
		if (Real && target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
			target->RemoveAurasDueToSpell(21009);

		// only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
		if (Real && !target->HasAuraType(SPELL_AURA_MOD_STEALTH))
		{
			// if no GM invisibility
			if (target->GetVisibility() != VISIBILITY_OFF)
			{
				target->RemoveStandFlags(UNIT_STAND_FLAGS_CREEP);

				if (target->GetTypeId() == TYPEID_PLAYER)
					target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

				// restore invisibility if any
				if (target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
				{
					target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
					target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
				}
				else
					target->SetVisibility(VISIBILITY_ON);
			}

			// apply delayed talent bonus remover at last stealth aura remove
			Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
			for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
			{
				// Master of Subtlety
				if ((*i)->GetSpellProto()->SpellIconID == 2114)
				{
					target->CastSpell(target, 31666, true);
					break;
				}
			}
		}
	}
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
	Unit* target = GetTarget();

	if (apply)
	{
		target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

		target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

		if (Real && target->GetTypeId() == TYPEID_PLAYER)
		{
			// apply glow vision
			target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);
		}

		// apply only if not in GM invisibility and not stealth
		if (target->GetVisibility() == VISIBILITY_ON)
		{
			// Aura not added yet but visibility code expect temporary add aura
			target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
			target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
		}
	}
	else
	{
		// recalculate value at modifier remove (current aura already removed)
		target->m_invisibilityMask = 0;
		Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
		for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
			target->m_invisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);

		// only at real aura remove and if not have different invisibility auras.
		if (Real && target->m_invisibilityMask == 0)
		{
			// remove glow vision
			if (target->GetTypeId() == TYPEID_PLAYER)
				target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

			// apply only if not in GM invisibility & not stealthed while invisible
			if (target->GetVisibility() != VISIBILITY_OFF)
			{
				// if have stealth aura then already have stealth visibility
				if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
					target->SetVisibility(VISIBILITY_ON);
			}
		}
	}
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
	Unit* target = GetTarget();

	if (apply)
	{
		target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
	}
	else
	{
		// recalculate value at modifier remove (current aura already removed)
		target->m_detectInvisibilityMask = 0;
		Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
		for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
			target->m_detectInvisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);
	}
	if (Real && target->GetTypeId() == TYPEID_PLAYER)
		((Player*)target)->GetCamera().UpdateVisibilityForOwner();
}

void Aura::HandleDetectAmore(bool apply, bool /*real*/)
{
	GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES2, 1, (PLAYER_FIELD_BYTE2_DETECT_AMORE_0 << m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (apply)
	{
		// Frost root aura -> freeze/unfreeze target
		if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
			target->ModifyAuraState(AURA_STATE_FROZEN, apply);

		target->addUnitState(UNIT_STAT_ROOT);
		target->SetTargetGuid(ObjectGuid());

		// Save last orientation
		if (target->getVictim())
			target->SetOrientation(target->GetAngle(target->getVictim()));

		if (target->GetTypeId() == TYPEID_PLAYER)
		{
			target->SetRoot(true);

			// Clear unit movement flags
			((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
		}
		else
			target->StopMoving();
	}
	else
	{
		// Frost root aura -> freeze/unfreeze target
		if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
		{
			bool found_another = false;
			for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
			{
				Unit::AuraList const& auras = target->GetAurasByType(*itr);
				for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
				{
					if (GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
					{
						found_another = true;
						break;
					}
				}
				if (found_another)
					break;
			}

			if (!found_another)
				target->ModifyAuraState(AURA_STATE_FROZEN, apply);
		}

		// Real remove called after current aura remove from lists, check if other similar auras active
		if (target->HasAuraType(SPELL_AURA_MOD_ROOT))
			return;

		target->clearUnitState(UNIT_STAT_ROOT);

		if (!target->hasUnitState(UNIT_STAT_STUNNED))     // prevent allow move if have also stun effect
		{
			if (target->getVictim() && target->isAlive())
				target->SetTargetGuid(target->getVictim()->GetObjectGuid());

			if (target->GetTypeId() == TYPEID_PLAYER)
				target->SetRoot(false);
		}
	}
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (apply)
	{
		target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
		// Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
		for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
			if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
				if (spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
					// Stop spells on prepare or casting state
						target->InterruptSpell(CurrentSpellTypes(i), false);

		switch (GetId())
		{
			// Arcane Torrent (Energy)
		case 25046:
			{
				Unit* caster = GetCaster();
				if (!caster)
					return;

				// Search Mana Tap auras on caster
				Aura* dummy = caster->GetDummyAura(28734);
				if (dummy)
				{
					int32 bp = dummy->GetStackAmount() * 10;
					caster->CastCustomSpell(caster, 25048, &bp, NULL, NULL, true);
					caster->RemoveAurasDueToSpell(28734);
				}
			}
		}
	}
	else
	{
		// Real remove called after current aura remove from lists, check if other similar auras active
		if (target->HasAuraType(SPELL_AURA_MOD_SILENCE))
			return;

		target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
	}
}

void Aura::HandleModThreat(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (!target->isAlive())
		return;

	int level_diff = 0;
	int multiplier = 0;
	switch (GetId())
	{
		// Arcane Shroud
	case 26400:
		level_diff = target->getLevel() - 60;
		multiplier = 2;
		break;
		// The Eye of Diminution
	case 28862:
		level_diff = target->getLevel() - 60;
		multiplier = 1;
		break;
	}

	if (level_diff > 0)
		m_modifier.m_amount += multiplier * level_diff;

	if (target->GetTypeId() == TYPEID_PLAYER)
		for (int8 x = 0; x < MAX_SPELL_SCHOOL; ++x)
			if (m_modifier.m_miscvalue & int32(1 << x))
				ApplyPercentModFloatVar(target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
		return;

	Unit* caster = GetCaster();

	if (!caster || !caster->isAlive())
		return;

	float threatMod = apply ? float(m_modifier.m_amount) : float(-m_modifier.m_amount);

	target->getHostileRefManager().threatAssist(caster, threatMod, GetSpellProto());
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
	// only at real add/remove aura
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (!target->isAlive() || !target->CanHaveThreatList())
		return;

	Unit* caster = GetCaster();

	if (!caster || !caster->isAlive())
		return;

	if (apply)
		target->TauntApply(caster);
	else
	{
		// When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
		target->TauntFadeOut(caster);
	}
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool apply, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool /*apply*/, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseFlightSpeed(bool apply, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	// Enable Fly mode for flying mounts
	if (m_modifier.m_auraname == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED)
	{
		WorldPacket data;
		if (apply)
			data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
		else
			data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
		data << target->GetPackGUID();
		data << uint32(0);                                  // unknown
		target->SendMessageToSet(&data, true);

		// Players on flying mounts must be immune to polymorph
		if (target->GetTypeId() == TYPEID_PLAYER)
			target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);

		// Dragonmaw Illusion (overwrite mount model, mounted aura already applied)
		if (apply && target->HasAura(42016, EFFECT_INDEX_0) && target->GetMountID())
			target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
	}

	target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	GetTarget()->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	if (apply)
	{
		// Gronn Lord's Grasp, becomes stoned
		if (GetId() == 33572)
		{
			if (GetStackAmount() >= 5 && !target->HasAura(33652))
				target->CastSpell(target, 33652, true);
		}
	}

	target->UpdateSpeed(MOVE_RUN, true);
	target->UpdateSpeed(MOVE_SWIM, true);
	target->UpdateSpeed(MOVE_FLIGHT, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	target->UpdateSpeed(MOVE_RUN, true);
	target->UpdateSpeed(MOVE_SWIM, true);
	target->UpdateSpeed(MOVE_FLIGHT, true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
	uint32 misc  = m_modifier.m_miscvalue;
	Unit* target = GetTarget();

	if (apply && GetSpellProto()->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))
	{
		uint32 mechanic = 1 << (misc - 1);

		// immune movement impairment and loss of control (spell data have special structure for mark this case)
		if (IsSpellRemoveAllMovementAndControlLossEffects(GetSpellProto()))
			mechanic = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

		target->RemoveAurasAtMechanicImmunity(mechanic, GetId());
	}

	target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, misc, apply);

	// special cases
	switch (misc)
	{
	case MECHANIC_INVULNERABILITY:
		target->ModifyAuraState(AURA_STATE_FORBEARANCE, apply);
		break;
	case MECHANIC_SHIELD:
		target->ModifyAuraState(AURA_STATE_WEAKENED_SOUL, apply);
		break;
	}

	// Bestial Wrath
	if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1680)
	{
		// The Beast Within cast on owner if talent present
		if (Unit* owner = target->GetOwner())
		{
			// Search talent The Beast Within
			Unit::AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
			for (Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
			{
				if ((*i)->GetSpellProto()->SpellIconID == 2229)
				{
					if (apply)
						owner->CastSpell(owner, 34471, true, NULL, this);
					else
						owner->RemoveAurasDueToSpell(34471);
					break;
				}
			}
		}
	}
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
	uint32 mechanic  = m_modifier.m_miscvalue;

	if (apply && GetSpellProto()->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))
		GetTarget()->RemoveAurasAtMechanicImmunity(mechanic, GetId());

	// check implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect
}

// this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();

	// Suffering passive immune health drain
	if(apply && GetId() == 41296)
		target->CastSpell(target, 41623, true);

	// when removing flag aura, handle flag drop
	if (!apply && target->GetTypeId() == TYPEID_PLAYER
		&& (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION))
	{
		Player* player = (Player*)target;
		if (BattleGround* bg = player->GetBattleGround())
			bg->EventPlayerDroppedFlag(player);
		else if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript(player->GetCachedZoneId()))
			outdoorPvP->HandleDropFlag(player, GetSpellProto()->Id);
	}

	target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
	if (apply && Real && GetSpellProto()->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))
	{
		Unit::AuraList const& auraList = GetTarget()->GetAurasByType(AuraType(m_modifier.m_miscvalue));
		for (Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
		{
			if (auraList.front() != this)                   // skip itself aura (it already added)
			{
				GetTarget()->RemoveAurasDueToSpell(auraList.front()->GetId());
				itr = auraList.begin();
			}
			else
				++itr;
		}
	}

	GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
	Unit* target = GetTarget();
	target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

	// remove all flag auras (they are positive, but they must be removed when you are immune)
	if (GetSpellProto()->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY) && GetSpellProto()->HasAttribute(SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD))
		target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

	// TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
	if (Real && apply
		&& GetSpellProto()->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
		&& IsPositiveSpell(GetId()))                    // Only positive immunity removes auras
	{
		uint32 school_mask = m_modifier.m_miscvalue;
		Unit::SpellAuraHolderMap& Auras = target->GetSpellAuraHolderMap();
		for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
		{
			next = iter;
			++next;
			SpellEntry const* spell = iter->second->GetSpellProto();
			if ((GetSpellSchoolMask(spell) & school_mask)   // Check for school mask
				&& !spell->HasAttribute(SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)   // Spells unaffected by invulnerability
				&& !iter->second->IsPositive()          // Don't remove positive spells
				&& spell->Id != GetId())                // Don't remove self
			{
				target->RemoveAurasDueToSpell(spell->Id);
				if (Auras.empty())
					break;
				else
					next = Auras.begin();
			}
		}
	}
	if (Real && GetSpellProto()->Mechanic == MECHANIC_BANISH)
	{
		if (apply)
			target->addUnitState(UNIT_STAT_ISOLATED);
		else
			target->clearUnitState(UNIT_STAT_ISOLATED);
	}
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
	GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);

	// Spite
	if(!apply && GetId() == 41376)
		GetCaster()->CastSpell(GetTarget(), 41377, true, NULL, this);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	switch (GetId())
	{
		// some spell have charges by functionality not have its in spell data
	case 28200:                                         // Ascendance (Talisman of Ascendance trinket)
		if (apply)
			GetHolder()->SetAuraCharges(6);
		break;
	default:
		break;
	}
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
	// used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
	if (apply)
		GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
	else
		GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;

	Unit* target = GetTarget();

	SpellEntry const* spell = GetSpellProto();

	if (apply)
	{
		switch(GetId())
		{
			//Totem shaman Effect apply effect when summoned
			// WF totem
		case 8515:	// Rank 1
		case 10609: // Rank 2
		case 10612:	// Rank 3	
		case 25581: // Rank 4
		case 25582: // Rank 5
			//case 8167:		// Poison totem
		case 8145:		// Seisme totem
		case 6474:		// Earthbind Totem
			TriggerSpell();
			break;
		}
	}
	else
	{
		switch (GetId())
		{
		case 66:                                        // Invisibility
			if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
				target->CastSpell(target, 32612, true, NULL, this);

			return;
		case 29213:                                     // Curse of the Plaguebringer
			if (m_removeMode != AURA_REMOVE_BY_DISPEL)
				// Cast Wrath of the Plaguebringer if not dispelled
					target->CastSpell(target, 29214, true, 0, this);
			return;
		case 42783:                                     // Wrath of the Astrom...
			if (m_removeMode == AURA_REMOVE_BY_EXPIRE && GetEffIndex() + 1 < MAX_EFFECT_INDEX)
				target->CastSpell(target, GetSpellProto()->CalculateSimpleValue(SpellEffectIndex(GetEffIndex() + 1)), true);

			return;
		default:
			break;
		}
	}
}

void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandlePeriodicEnergize(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandleAuraPeriodicDummy(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	// For prevent double apply bonuses
	bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

	SpellEntry const* spell = GetSpellProto();
	switch (spell->SpellFamilyName)
	{
	case SPELLFAMILY_ROGUE:
		{
			// Master of Subtlety
			if (spell->Id == 31666 && !apply)
			{
				target->RemoveAurasDueToSpell(31665);
				break;
			}
			break;
		}
	case SPELLFAMILY_HUNTER:
		{
			// Aspect of the Viper
			if (spell->SpellFamilyFlags & UI64LIT(0x0004000000000000))
			{
				// Update regen on remove
				if (!apply && target->GetTypeId() == TYPEID_PLAYER)
					((Player*)target)->UpdateManaRegen();
				break;
			}
			break;
		}
	}

	m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;

	Unit* target = GetTarget();

	// For prevent double apply bonuses
	bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

	// Custom damage calculation after
	if (apply)
	{
		if (loading)
			return;

		Unit* caster = GetCaster();
		if (!caster)
			return;

		m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
	}
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	m_isPeriodic = apply;

	Unit* target = GetTarget();
	SpellEntry const* spellProto = GetSpellProto();

	// For prevent double apply bonuses
	bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

	// Custom damage calculation after
	if (apply)
	{
		if (loading)
			return;

		Unit* caster = GetCaster();
		if (!caster)
			return;

		switch (spellProto->SpellFamilyName)
		{
		case SPELLFAMILY_GENERIC:
			{
				if(spellProto->Id == 40953)
				{					
					int32 dmg = m_currentBasePoints;
					error_log("*********** DAMAGE TICK %u ***********", m_currentBasePoints);
				}
			break;
			}
		case SPELLFAMILY_WARRIOR:
			{
				// Rend
				if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000020))
				{
					// 0.00743*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
					float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
					int32 mws = caster->GetAttackTime(BASE_ATTACK);
					float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
					float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
					m_modifier.m_amount += int32(((mwb_min + mwb_max) / 2 + ap * mws / 14000) * 0.00743f);
				}
				break;
			}
		case SPELLFAMILY_DRUID:
			{
				// Rip
				if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000800000))
				{
					if (caster->GetTypeId() != TYPEID_PLAYER)
						break;

					// $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]

					uint8 cp = ((Player*)caster)->GetComboPoints();

					// Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
					Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
					for (Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
					{
						if ((*itr)->GetId() == 34241)
						{
							m_modifier.m_amount += cp * (*itr)->GetModifier()->m_amount;
							break;
						}
					}

					if (cp > 4) cp = 4;
					m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
				}
				break;
			}
		case SPELLFAMILY_ROGUE:
			{
				// Rupture
				if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000100000))
				{
					if (caster->GetTypeId() != TYPEID_PLAYER)
						break;
					// Dmg/tick = $AP*min(0.01*$cp, 0.03) [Like Rip: only the first three CP increase the contribution from AP]
					uint8 cp = ((Player*)caster)->GetComboPoints();
					if (cp > 3) cp = 3;
					m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
				}
				break;
			}
		default:
			break;
		}

		if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
		{
			// SpellDamageBonusDone for magic spells
			if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
				m_modifier.m_amount = caster->SpellDamageBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
			// MeleeDamagebonusDone for weapon based spells
			else
			{
				WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
				m_modifier.m_amount = caster->MeleeDamageBonusDone(target, m_modifier.m_amount, attackType, GetSpellProto(), DOT, GetStackAmount());
			}
		}
	}
	// remove time effects
	else
	{
		// Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
		if (spellProto->Id == 41917)
			target->CastSpell(target, 41915, true);
		// Skeleton Shot - Summoning Skeleton if target die
		else if (spellProto->Id == 41171)
		{
			if(m_removeMode == AURA_REMOVE_BY_DEATH)
				target->CastSpell(target, 41174, true);
		}
	}
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;

	// For prevent double apply bonuses
	bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

	// Custom damage calculation after
	if (apply)
	{
		if (loading)
			return;

		Unit* caster = GetCaster();
		if (!caster)
			return;

		m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
	}
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;

	// For prevent double apply bonuses
	bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

	// Custom damage calculation after
	if (apply)
	{
		if (loading)
			return;

		Unit* caster = GetCaster();
		if (!caster)
			return;

		m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
	}
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
	for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
	{
		if (m_modifier.m_miscvalue & int32(1 << x))
		{
			GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply);
			if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
				GetTarget()->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
		}
	}
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
	for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
	{
		if (m_modifier.m_miscvalue & int32(1 << x))
		{
			GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
			if (GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->IsPet())
				GetTarget()->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
		}
	}
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
	// only players have base stats
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
	{
		// pets only have base armor
		if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
			GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
	}
	else
	{
		for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
		{
			if (m_modifier.m_miscvalue & int32(1 << x))
				GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
		}
	}
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();

	for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
	{
		if (m_modifier.m_miscvalue & int32(1 << i))
		{
			target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
			if (target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->IsPet())
			{
				target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
				target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
			}
		}
	}
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
	// only players have base stats
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
	{
		// only pets have base stats
		if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
			GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
	}
	else
	{
		for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
			if (m_modifier.m_miscvalue & (1 << i))
				GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
	}
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
	if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
	{
		sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), m_modifier.m_miscvalue);
		return;
	}

	// Holy Strength amount decrease by 4% each level after 60
	if(apply && GetId() == 20007)
		if(GetCaster()->GetTypeId() == TYPEID_PLAYER && GetCaster()->getLevel() > 60)
			m_modifier.m_amount = int32(m_modifier.m_amount * (1 - (((float(GetCaster()->getLevel()) - 60) * 4) / 100)));

	for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
	{
		// -1 or -2 is all stats ( misc < -2 checked in function beginning )
		if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
		{
			// m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
			GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
			if (GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->IsPet())
				GetTarget()->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
		}
	}
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
	if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
	{
		sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
		return;
	}

	// only players have base stats
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
	{
		if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
			GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
	}
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Magic damage modifiers implemented in Unit::SpellDamageBonusDone
	// This information for client side use only
	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleAuraModDispelResist(bool apply, bool Real)
{
	if (!Real || !apply)
		return;

	if (GetId() == 33206)
		GetTarget()->CastSpell(GetTarget(), 44416, true, NULL, this, GetCasterGuid());
}

void Aura::HandleModSpellDamagePercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Magic damage modifiers implemented in Unit::SpellDamageBonusDone
	// This information for client side use only
	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;
	// implemented in Unit::SpellHealingBonusDone
	// this information is for client side only
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
	if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
	{
		sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
		return;
	}

	Unit* target = GetTarget();

	// save current and max HP before applying aura
	uint32 curHPValue = target->GetHealth();
	uint32 maxHPValue = target->GetMaxHealth();

	for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
	{
		if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
		{
			target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
			if (target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->IsPet())
				target->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply);
		}
	}

	// recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
	if (m_modifier.m_miscvalue == STAT_STAMINA && maxHPValue > 0 && GetSpellProto()->HasAttribute(SPELL_ATTR_UNK4))
	{
		// newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
		uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
		target->SetHealth(newHPValue);
	}
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
	{
		// support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
		// and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
		sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
		return;
	}

	// Recalculate Armor
	GetTarget()->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
	m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
	if (m_modifier.periodictime == 0)
		m_modifier.periodictime = 1000;

	m_periodicTimer = m_modifier.periodictime;
	m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
	if (m_modifier.periodictime == 0)
		m_modifier.periodictime = 5000;

	m_periodicTimer = 5000;
	m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
	if (!Real)
		return;

	Powers pt = GetTarget()->getPowerType();
	if (m_modifier.periodictime == 0)
	{
		// Anger Management (only spell use this aura for rage)
		if (pt == POWER_RAGE)
			m_modifier.periodictime = 3000;
		else
			m_modifier.periodictime = 2000;
	}

	m_periodicTimer = 5000;

	if (GetTarget()->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
		((Player*)GetTarget())->UpdateManaRegen();

	m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Update manaregen value
	if (m_modifier.m_miscvalue == POWER_MANA)
		((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleModManaRegen(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Note: an increase in regen does NOT cause threat.
	((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleComprehendLanguage(bool apply, bool /*Real*/)
{
	if (apply)
		GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
	else
		GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
	Unit* target = GetTarget();

	switch (GetId())
	{
		// Special case with temporary increase max/current health
		// Cases where we need to manually calculate the amount for the spell (by percentage)
		// recalculate to full amount at apply for proper remove
		// Backport notive TBC: no cases yet
		// no break here

		// Cases where m_amount already has the correct value (spells cast with CastCustomSpell or absolute values)
	case 12976:                                         // Warrior Last Stand triggered spell (Cast with percentage-value by CastCustomSpell)
	case 28726:                                         // Nightmare Seed
	case 34511:                                         // Valor (Bulwark of Kings, Bulwark of the Ancient Kings)
	case 44055:                                         // Tremendous Fortitude (Battlemaster's Alacrity)
	case 40604:											// Fel Rage( Gurtogg )
		{
			if (Real)
			{
				if (apply)
				{
					target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
					target->ModifyHealth(m_modifier.m_amount);
				}
				else
				{
					if (int32(target->GetHealth()) > m_modifier.m_amount)
						target->ModifyHealth(-m_modifier.m_amount);
					else
						target->SetHealth(1);
					target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
				}
			}
			return;
		}
		// generic case
	default:
		target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
	}
}

void  Aura::HandleAuraModIncreaseMaxHealth(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	uint32 oldhealth = target->GetHealth();
	double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

	target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);

	// refresh percentage
	if (oldhealth > 0)
	{
		uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
		if (newhealth == 0)
			newhealth = 1;

		target->SetHealth(newhealth);
	}
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	Powers powerType = target->getPowerType();
	if (int32(powerType) != m_modifier.m_miscvalue)
		return;

	UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

	target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
	Powers powerType = GetTarget()->getPowerType();
	if (int32(powerType) != m_modifier.m_miscvalue)
		return;

	UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

	GetTarget()->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
	GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)GetTarget())->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)GetTarget())->UpdateDodgePercentage();
	// sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)GetTarget())->UpdateBlockPercentage();
	// sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
	Unit* target = GetTarget();

	if (target->GetTypeId() != TYPEID_PLAYER)
		return;

	// apply item specific bonuses for already equipped weapon
	if (Real)
	{
		for (int i = 0; i < MAX_ATTACK; ++i)
			if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
				((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
	}

	// mods must be applied base at equipped weapon class and subclass comparison
	// with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
	// m_modifier.m_miscvalue comparison with item generated damage types

	if (GetSpellProto()->EquippedItemClass == -1)
	{
		((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float(m_modifier.m_amount), apply);
		((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
		((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float(m_modifier.m_amount), apply);
	}
	else
	{
		// done in Player::_ApplyWeaponDependentAuraMods
	}
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();

	if (target->GetTypeId() == TYPEID_PLAYER)
	{
		((Player*)target)->UpdateMeleeHitChances();
		((Player*)target)->UpdateRangedHitChances();
	}
	else
	{
		target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
		target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
	}
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
	{
		((Player*)GetTarget())->UpdateSpellHitChances();
	}
	else
	{
		GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
	}
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
	{
		((Player*)GetTarget())->UpdateAllSpellCritChances();
	}
	else
	{
		GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
	}
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
		if (m_modifier.m_miscvalue & (1 << school))
			((Player*)GetTarget())->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
	GetTarget()->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
}

void Aura::HandleModMeleeRangedSpeedPct(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModCombatSpeedPct(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	target->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
	GetTarget()->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModMeleeSpeedPct(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
	GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;
	GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
	GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
	if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
		return;

	GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
	// UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
	GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
	if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
		return;

	// UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
	GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleModMeleeAttackPowerAttackerBonus(bool apply, bool /*Real*/)
{
	if (!apply)
		return;

	switch(GetId())
	{
	case 1130:
	case 14323:
	case 14324:
	case 14325:
		if(Unit* caster = GetCaster())
		{
			Unit::AuraList const& mclassScritAuras = caster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
			for (Unit::AuraList::const_iterator j = mclassScritAuras.begin(); j != mclassScritAuras.end(); ++j)
			{
				switch((*j)->GetModifier()->m_miscvalue)
				{
				case 5240:
				case 5237:
				case 5238:
				case 5236:
				case 5239:
					m_modifier.m_amount = (*j)->GetModifier()->m_amount;
					break;
				default : break;
				}
			}
		}
		break;
	default : break;
	}

}

void Aura::HandleAuraModRangedAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	// Recalculate bonus
	if (GetTarget()->GetTypeId() == TYPEID_PLAYER && !(GetTarget()->getClassMask() & CLASSMASK_WAND_USERS))
		((Player*)GetTarget())->UpdateAttackPowerAndDamage(true);
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
	Unit* target = GetTarget();

	// apply item specific bonuses for already equipped weapon
	if (Real && target->GetTypeId() == TYPEID_PLAYER)
	{
		for (int i = 0; i < MAX_ATTACK; ++i)
			if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
				((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
	}

	// m_modifier.m_miscvalue is bitmask of spell schools
	// 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
	// 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
	// 127 - full bitmask any damages
	//
	// mods must be applied base at equipped weapon class and subclass comparison
	// with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
	// m_modifier.m_miscvalue comparison with item generated damage types

	if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
	{
		// apply generic physical damage bonuses including wand case
		if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
		{
			target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
			target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
			target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
		}
		else
		{
			// done in Player::_ApplyWeaponDependentAuraMods
		}

		if (target->GetTypeId() == TYPEID_PLAYER)
		{
			if (m_positive)
				target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
			else
				target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
		}
	}

	// Skip non magic case for speedup
	if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
		return;

	if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
	{
		// wand magic case (skip generic to all item spell bonuses)
		// done in Player::_ApplyWeaponDependentAuraMods

		// Skip item specific requirements for not wand magic damage
		return;
	}

	// Magic damage modifiers implemented in Unit::SpellDamageBonusDone
	// This information for client side use only
	if (target->GetTypeId() == TYPEID_PLAYER)
	{
		if (m_positive)
		{
			for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
			{
				if ((m_modifier.m_miscvalue & (1 << i)) != 0)
					target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
			}
		}
		else
		{
			for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
			{
				if ((m_modifier.m_miscvalue & (1 << i)) != 0)
					target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
			}
		}
		Pet* pet = target->GetPet();
		if (pet)
			pet->UpdateAttackPowerAndDamage();
	}
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
	Unit* target = GetTarget();

	// apply item specific bonuses for already equipped weapon
	if (Real && target->GetTypeId() == TYPEID_PLAYER)
	{
		for (int i = 0; i < MAX_ATTACK; ++i)
			if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
				((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
	}

	// m_modifier.m_miscvalue is bitmask of spell schools
	// 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
	// 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
	// 127 - full bitmask any damages
	//
	// mods must be applied base at equipped weapon class and subclass comparison
	// with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
	// m_modifier.m_miscvalue comparison with item generated damage types

	if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
	{
		// apply generic physical damage bonuses including wand case
		if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
		{
			target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
			target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
			target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
		}
		else
		{
			// done in Player::_ApplyWeaponDependentAuraMods
		}
		// For show in client
		if (target->GetTypeId() == TYPEID_PLAYER)
			target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount / 100.0f, apply);
	}

	// Skip non magic case for speedup
	if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
		return;

	if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
	{
		// wand magic case (skip generic to all item spell bonuses)
		// done in Player::_ApplyWeaponDependentAuraMods

		// Skip item specific requirements for not wand magic damage
		return;
	}

	// Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
	// Send info to client
	if (target->GetTypeId() == TYPEID_PLAYER)
		for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
			target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount / 100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

	GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	float amount = m_modifier.m_amount / 100.0f;
	for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
		if (m_modifier.m_miscvalue & (1 << i))
			GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
		if (m_modifier.m_miscvalue & (1 << i))
			GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleShapeshiftBoosts(bool apply)
{
	uint32 spellId1 = 0;
	uint32 spellId2 = 0;
	uint32 HotWSpellId = 0;

	ShapeshiftForm form = ShapeshiftForm(GetModifier()->m_miscvalue);

	Unit* target = GetTarget();

	switch (form)
	{
	case FORM_CAT:
		spellId1 = 3025;
		HotWSpellId = 24900;
		break;
	case FORM_TREE:
		spellId1 = 5420;
		break;
	case FORM_TRAVEL:
		spellId1 = 5419;
		break;
	case FORM_AQUA:
		spellId1 = 5421;
		break;
	case FORM_BEAR:
		spellId1 = 1178;
		spellId2 = 21178;
		HotWSpellId = 24899;
		break;
	case FORM_DIREBEAR:
		spellId1 = 9635;
		spellId2 = 21178;
		HotWSpellId = 24899;
		break;
	case FORM_BATTLESTANCE:
		spellId1 = 21156;
		break;
	case FORM_DEFENSIVESTANCE:
		spellId1 = 7376;
		break;
	case FORM_BERSERKERSTANCE:
		spellId1 = 7381;
		break;
	case FORM_MOONKIN:
		spellId1 = 24905;
		break;
	case FORM_FLIGHT:
		spellId1 = 33948;
		spellId2 = 34764;
		break;
	case FORM_FLIGHT_EPIC:
		spellId1 = 40122;
		spellId2 = 40121;
		break;
	case FORM_SPIRITOFREDEMPTION:
		spellId1 = 27792;
		spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
		break;
	case FORM_GHOSTWOLF:
	case FORM_AMBIENT:
	case FORM_GHOUL:
	case FORM_SHADOW:
	case FORM_STEALTH:
	case FORM_CREATURECAT:
	case FORM_CREATUREBEAR:
		break;
	}

	if (apply)
	{
		if (spellId1)
			target->CastSpell(target, spellId1, true, NULL, this);
		if (spellId2)
			target->CastSpell(target, spellId2, true, NULL, this);

		if (target->GetTypeId() == TYPEID_PLAYER)
		{
			const PlayerSpellMap& sp_list = ((Player*)target)->GetSpellMap();
			for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
			{
				if (itr->second.state == PLAYERSPELL_REMOVED) continue;
				if (itr->first == spellId1 || itr->first == spellId2) continue;
				SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
				if (!spellInfo || !IsNeedCastSpellAtFormApply(spellInfo, form))
					continue;
				target->CastSpell(target, itr->first, true, NULL, this);
			}

			// Leader of the Pack
			if (((Player*)target)->HasSpell(17007))
			{
				SpellEntry const* spellInfo = sSpellStore.LookupEntry(24932);
				if (spellInfo && spellInfo->Stances & (1 << (form - 1)))
					target->CastSpell(target, 24932, true, NULL, this);
			}

			// Heart of the Wild
			if (HotWSpellId)
			{
				Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
				for (Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
				{
					if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
					{
						int32 HotWMod = (*i)->GetModifier()->m_amount;
						if (GetModifier()->m_miscvalue == FORM_CAT)
							HotWMod /= 2;

						target->CastCustomSpell(target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
						break;
					}
				}
			}
		}
	}
	else
	{
		if (spellId1)
			target->RemoveAurasDueToSpell(spellId1);
		if (spellId2)
			target->RemoveAurasDueToSpell(spellId2);

		Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
		for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
		{
			if (itr->second->IsRemovedOnShapeLost())
			{
				target->RemoveAurasDueToSpell(itr->second->GetId());
				itr = tAuras.begin();
			}
			else
				++itr;
		}
	}
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_UNIT)
		return;

	CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(GetTarget()->GetEntry());
	if (ci && ci->type == CREATURE_TYPE_BEAST)
		GetTarget()->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
	if (apply)
		GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
	else
		GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
	if (apply)
		GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
	else
		GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
	HandleAuraModPacify(apply, Real);
	HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	if (apply)
	{
		GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
	}
	else
	{
		GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
	}
}

void Aura::HandleAuraAllowFlight(bool apply, bool Real)
{
	// all applied/removed only at real aura add/remove
	if (!Real)
		return;

	// allow fly
	WorldPacket data;
	if (apply)
		data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
	else
		data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
	data << GetTarget()->GetPackGUID();
	data << uint32(0);                                      // unk
	GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleModRating(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
		if (m_modifier.m_miscvalue & (1 << rating))
			((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), m_modifier.m_amount, apply);
}

void Aura::HandleForceMoveForward(bool apply, bool Real)
{
	if (!Real)
		return;

	if (apply)
		GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
	else
		GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
}

void Aura::HandleAuraModExpertise(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	((Player*)GetTarget())->UpdateExpertise(BASE_ATTACK);
	((Player*)GetTarget())->UpdateExpertise(OFF_ATTACK);
}

void Aura::HandleModTargetResistance(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;
	Unit* target = GetTarget();

	// Warp-Spring Coil (item 30450) @Kordbc
	if(GetId() == 37174)
		GetHolder()->SetAuraCharges(0);

	// applied to damage as HandleNoImmediateEffect in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
	// show armor penetration
	if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
		target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, m_modifier.m_amount, apply);

	// show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
	if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_SPELL) == SPELL_SCHOOL_MASK_SPELL)
		target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, m_modifier.m_amount, apply);
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
	BaseModType modType = FLAT_MOD;
	if (m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
		modType = PCT_MOD;

	if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
		((Player*)GetTarget())->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	Player* target = (Player*)GetTarget();

	// combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
	// remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
	if (!apply && m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetComboTargetGuid())
		if (Unit* unit = ObjectAccessor::GetUnit(*GetTarget(), target->GetComboTargetGuid()))
			target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable(bool Apply, bool Real)
{
	if (Real && Apply)
	{
		GetTarget()->CombatStop();
		GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
	}
	GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, Apply);
}

void Aura::HandleSpiritOfRedemption(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	// prepare spirit state
	if (apply)
	{
		if (target->GetTypeId() == TYPEID_PLAYER)
		{
			// disable breath/etc timers
			((Player*)target)->StopMirrorTimers();

			// set stand state (expected in this form)
			if (!target->IsStandState())
				target->SetStandState(UNIT_STAND_STATE_STAND);
		}

		target->SetHealth(1);
	}
	// die at aura end
	else
		target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* caster = GetCaster();
	if (!caster)
		return;

	Unit* target = GetTarget();
	SpellEntry const* spellProto = GetSpellProto();
	if (apply)
	{
		// prevent double apply bonuses
		if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
		{
			float DoneActualBenefit = 0.0f;
			switch (spellProto->SpellFamilyName)
			{
			case SPELLFAMILY_PRIEST:
				// Power Word: Shield
				if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
				{
					//+30% from +healing bonus
					DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.3f;
					break;
				}
				break;
			case SPELLFAMILY_MAGE:
				// Frost Ward, Fire Ward
				if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000100080108)))
					//+10% from +spell bonus
						DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
				break;
			case SPELLFAMILY_WARLOCK:
				// Shadow Ward
				if (spellProto->SpellFamilyFlags == UI64LIT(0x00))
					//+10% from +spell bonus
						DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
				break;
			default:
				break;
			}

			DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

			m_modifier.m_amount += (int32)DoneActualBenefit;
		}
	}
	else
	{
		if (GetId() == 40251)
		{
			// Shadow Of Death Summon Skeleton & Spirit (Teron)
			target->CastSpell(target, 40270, true);
			target->CastSpell(target, 41948, true);
			target->CastSpell(target, 41949, true);
			target->CastSpell(target, 41950, true);
			target->CastSpell(target, 40266, true);
		}
	}
}

void Aura::PeriodicTick()
{
	Unit* target = GetTarget();
	SpellEntry const* spellProto = GetSpellProto();

	switch (m_modifier.m_auraname)
	{
	case SPELL_AURA_PERIODIC_DAMAGE:
	case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
		{
			// don't damage target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			Unit* pCaster = GetCaster();
			if (!pCaster)
				return;

			if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
				pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
				return;

			// Check for immune (not use charges)
			if (target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
				return;

			// some auras remove at specific health level or more
			if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
			{
				switch (GetId())
				{
				case 43093: case 31956: case 38801:
				case 35321: case 38363: case 39215:
				case 48920:
					{
						if (target->GetHealth() == target->GetMaxHealth())
						{
							target->RemoveAurasDueToSpell(GetId());
							return;
						}
						break;
					}
				case 38772:
					{
						uint32 percent =
							GetEffIndex() < EFFECT_INDEX_2 && spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_DUMMY ?
							pCaster->CalculateSpellDamage(target, spellProto, SpellEffectIndex(GetEffIndex() + 1)) :
							100;
						if (target->GetHealth() * 100 >= target->GetMaxHealth() * percent)
						{
							target->RemoveAurasDueToSpell(GetId());
							return;
						}
						break;
					}
				default:
					break;
				}
			}

			uint32 absorb = 0;
			uint32 resist = 0;
			CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL);

			// ignore non positive values (can be result apply spellmods to aura damage
			uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			uint32 pdamage;

			if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
				pdamage = amount;
			else
				pdamage = uint32(target->GetMaxHealth() * amount / 100);

			// SpellDamageBonus for magic spells
			if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
				pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());
			// MeleeDamagebonus for weapon based spells
			else
			{
				WeaponAttackType attackType = GetWeaponAttackType(spellProto);
				pdamage = target->MeleeDamageBonusTaken(pCaster, pdamage, attackType, spellProto, DOT, GetStackAmount());
			}

			// Calculate armor mitigation if it is a physical spell
			// But not for bleed mechanic spells
			if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL &&
				GetEffectMechanic(spellProto, m_effIndex) != MECHANIC_BLEED)
			{
				uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
				cleanDamage.damage += pdamage - pdamageReductedArmor;
				pdamage = pdamageReductedArmor;
			}

			// Curse of Agony damage-per-tick calculation
			if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400)) && spellProto->SpellIconID == 544)
			{
				// 1..4 ticks, 1/2 from normal tick damage
				if (GetAuraTicks() <= 4)
					pdamage = pdamage / 2;
				// 9..12 ticks, 3/2 from normal tick damage
				else if (GetAuraTicks() >= 9)
					pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
				// 5..8 ticks have normal tick damage
			}

			// Aura of Anger (ROS)
			if(spellProto->Id == 41337)
			{
				pdamage = GetBasePoints() * GetAuraTicks();
				if(Aura* AuraAngerEff1 = GetHolder()->GetAuraByEffectIndex(EFFECT_INDEX_1))
					AuraAngerEff1->GetHolder()->SetStackAmount(AuraAngerEff1->GetHolder()->GetStackAmount() + 1);
			}

			// As of 2.2 resilience reduces damage from DoT ticks as much as the chance to not be critically hit
			// Reduce dot damage from resilience for players
			if (target->GetTypeId() == TYPEID_PLAYER)
				pdamage -= ((Player*)target)->GetDotDamageReduction(pdamage);
			target->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !GetSpellProto()->HasAttribute(SPELL_ATTR_EX2_CANT_REFLECTED));
			if(IsSpellBinary(spellProto, pCaster))
				resist = 0;

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s attacked %s for %u dmg inflicted by %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

			pCaster->DealDamageMods(target, pdamage, &absorb);

			// Set trigger flag
			uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
			uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
			pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);

			SpellPeriodicAuraLogInfo pInfo(this, pdamage, absorb, resist, 0.0f);
			target->SendPeriodicAuraLog(&pInfo);

			if (pdamage)
				procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

			pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);

			pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, true);
			break;
		}
	case SPELL_AURA_PERIODIC_LEECH:
	case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
		{
			// don't damage target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			Unit* pCaster = GetCaster();
			if (!pCaster)
				return;

			if (!pCaster->isAlive())
				return;

			if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
				pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
				return;

			// Check for immune
			if (target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
				return;

			uint32 absorb = 0;
			uint32 resist = 0;
			CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL);

			uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			// Calculate armor mitigation if it is a physical spell
			if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL)
			{
				uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
				cleanDamage.damage += pdamage - pdamageReductedArmor;
				pdamage = pdamageReductedArmor;
			}

			pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

			// As of 2.2 resilience reduces damage from DoT ticks as much as the chance to not be critically hit
			// Reduce dot damage from resilience for players
			if (target->GetTypeId() == TYPEID_PLAYER)
				pdamage -= ((Player*)target)->GetDotDamageReduction(pdamage);

			target->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !spellProto->HasAttribute(SPELL_ATTR_EX2_CANT_REFLECTED));

			if (target->GetHealth() < pdamage)
				pdamage = uint32(target->GetHealth());

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s health leech of %s for %u dmg inflicted by %u abs is %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId(), absorb);

			pCaster->DealDamageMods(target, pdamage, &absorb);

			pCaster->SendSpellNonMeleeDamageLog(target, GetId(), pdamage, GetSpellSchoolMask(spellProto), absorb, resist, false, 0);

			float multiplier = spellProto->EffectMultipleValue[GetEffIndex()] > 0 ? spellProto->EffectMultipleValue[GetEffIndex()] : 1;

			// Set trigger flag
			uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; // | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
			uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;

			pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);
			if (pdamage)
				procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

			pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);
			int32 new_damage = pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, false);

			if (!target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
				for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
					if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
						if (spell->m_spellInfo->Id == GetId())
							spell->cancel();

			if (Player* modOwner = pCaster->GetSpellModOwner())
				modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

			uint32 heal = pCaster->SpellHealingBonusTaken(pCaster, spellProto, int32(new_damage * multiplier), DOT, GetStackAmount());

			int32 gain = pCaster->DealHeal(pCaster, heal, spellProto);
			pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
			break;
		}
	case SPELL_AURA_PERIODIC_HEAL:
	case SPELL_AURA_OBS_MOD_HEALTH:
		{
			// don't heal target if not alive, mostly death persistent effects from items
			if (!target->isAlive())
				return;

			Unit* pCaster = GetCaster();
			if (!pCaster)
				return;

			// heal for caster damage (must be alive)
			if (target != pCaster && spellProto->SpellVisual == 163 && !pCaster->isAlive())
				return;

			// ignore non positive values (can be result apply spellmods to aura damage
			uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			uint32 pdamage;

			if (m_modifier.m_auraname == SPELL_AURA_OBS_MOD_HEALTH)
				pdamage = uint32(target->GetMaxHealth() * amount / 100);
			else
				pdamage = amount;

			pdamage = target->SpellHealingBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s heal of %s for %u health inflicted by %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

			int32 gain = target->ModifyHealth(pdamage);
			SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
			target->SendPeriodicAuraLog(&pInfo);

			// Set trigger flag
			uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
			uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
			uint32 procEx = PROC_EX_NORMAL_HIT | PROC_EX_PERIODIC_POSITIVE;
			pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, gain, BASE_ATTACK, spellProto);

			// add HoTs to amount healed in bgs
			if (pCaster->GetTypeId() == TYPEID_PLAYER)
				if (BattleGround* bg = ((Player*)pCaster)->GetBattleGround())
					bg->UpdatePlayerScore(((Player*)pCaster), SCORE_HEALING_DONE, gain);

			target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);

			// heal for caster damage
			if (target != pCaster && spellProto->SpellVisual == 163)
			{
				uint32 dmg = spellProto->manaPerSecond;
				if (pCaster->GetHealth() <= dmg && pCaster->GetTypeId() == TYPEID_PLAYER)
				{
					pCaster->RemoveAurasDueToSpell(GetId());

					// finish current generic/channeling spells, don't affect autorepeat
					pCaster->FinishSpell(CURRENT_GENERIC_SPELL);
					pCaster->FinishSpell(CURRENT_CHANNELED_SPELL);
				}
				else
				{
					uint32 damage = gain;
					uint32 absorb = 0;
					pCaster->DealDamageMods(pCaster, damage, &absorb);
					pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), damage, GetSpellSchoolMask(spellProto), absorb, 0, false, 0, false);

					CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL);
					pCaster->DealDamage(pCaster, damage, &cleanDamage, NODAMAGE, GetSpellSchoolMask(spellProto), spellProto, true);
				}
			}
			break;
		}
	case SPELL_AURA_PERIODIC_MANA_LEECH:
		{
			// don't damage target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
				return;

			Powers power = Powers(m_modifier.m_miscvalue);

			// power type might have changed between aura applying and tick (druid's shapeshift)
			if (target->getPowerType() != power)
				return;

			Unit* pCaster = GetCaster();
			if (!pCaster)
				return;

			if (!pCaster->isAlive())
				return;

			if (GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
				pCaster->SpellHitResult(target, spellProto, false) != SPELL_MISS_NONE)
				return;

			// Check for immune (not use charges)
			if (target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
				return;

			// ignore non positive values (can be result apply spellmods to aura damage
			uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s power leech of %s for %u dmg inflicted by %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

			int32 drain_amount = target->GetPower(power) > pdamage ? pdamage : target->GetPower(power);

			// resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
			if (power == POWER_MANA && target->GetTypeId() == TYPEID_PLAYER)
				drain_amount -= ((Player*)target)->GetSpellCritDamageReduction(drain_amount);

			target->ModifyPower(power, -drain_amount);

			float gain_multiplier = 0.0f;

			if (pCaster->GetMaxPower(power) > 0)
			{
				gain_multiplier = spellProto->EffectMultipleValue[GetEffIndex()];

				if (Player* modOwner = pCaster->GetSpellModOwner())
					modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
			}

			SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, gain_multiplier);
			target->SendPeriodicAuraLog(&pInfo);

			if (int32 gain_amount = int32(drain_amount * gain_multiplier))
			{
				int32 gain = pCaster->ModifyPower(power, gain_amount);

				if (GetId() == 5138)                        // Drain Mana
					if (Aura* petPart = GetHolder()->GetAuraByEffectIndex(EFFECT_INDEX_1))
						if (int pet_gain = gain_amount * petPart->GetModifier()->m_amount / 100)
							pCaster->CastCustomSpell(pCaster, 32554, &pet_gain, NULL, NULL, true);

				target->AddThreat(pCaster, float(gain) * 0.5f, false, GetSpellSchoolMask(spellProto), spellProto);
			}

			// Some special cases
			switch (GetId())
			{
			case 32960:                                 // Mark of Kazzak
				{
					if (target->GetTypeId() == TYPEID_PLAYER && target->getPowerType() == POWER_MANA)
					{
						// Drain 5% of target's mana
						pdamage = target->GetMaxPower(POWER_MANA) * 5 / 100;
						drain_amount = target->GetPower(POWER_MANA) > pdamage ? pdamage : target->GetPower(POWER_MANA);
						target->ModifyPower(POWER_MANA, -drain_amount);

						SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, 0.0f);
						target->SendPeriodicAuraLog(&pInfo);
					}
					// no break here
				}
			case 21056:                                 // Mark of Kazzak
			case 31447:                                 // Mark of Kaz'rogal
				{
					uint32 triggerSpell = 0;
					switch (GetId())
					{
					case 21056: triggerSpell = 21058; break;
					case 31447: triggerSpell = 31463; break;
					case 32960: triggerSpell = 32961; break;
					}
					if (target->GetTypeId() == TYPEID_PLAYER && target->GetPower(power) == 0)
					{
						target->CastSpell(target, triggerSpell, true, NULL, this);
						target->RemoveAurasDueToSpell(GetId());
					}
					break;
				}
			}
			break;
		}
	case SPELL_AURA_PERIODIC_ENERGIZE:
		{
			// don't energize target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			// ignore non positive values (can be result apply spellmods to aura damage
			uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u dmg inflicted by %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

			if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
				break;

			Powers power = Powers(m_modifier.m_miscvalue);

			if (target->GetMaxPower(power) == 0)
				break;

			SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
			target->SendPeriodicAuraLog(&pInfo);

			int32 gain = target->ModifyPower(power, pdamage);

			if (Unit* pCaster = GetCaster())
				pCaster->getHostileRefManager().threatAssist(target, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
			break;
		}
	case SPELL_AURA_OBS_MOD_MANA:
		{
			// don't energize target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			// ignore non positive values (can be result apply spellmods to aura damage
			uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			uint32 pdamage = uint32(target->GetMaxPower(POWER_MANA) * amount / 100);

			DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u mana inflicted by %u",
				GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

			if (target->GetMaxPower(POWER_MANA) == 0)
				break;

			SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
			target->SendPeriodicAuraLog(&pInfo);

			int32 gain = target->ModifyPower(POWER_MANA, pdamage);

			if (Unit* pCaster = GetCaster())
				target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
			break;
		}
	case SPELL_AURA_POWER_BURN_MANA:
		{
			// don't mana burn target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			Unit* pCaster = GetCaster();
			if (!pCaster)
				return;

			// Check for immune (not use charges)
			if (target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
				return;

			int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

			Powers powerType = Powers(m_modifier.m_miscvalue);

			if (!target->isAlive() || target->getPowerType() != powerType)
				return;

			// resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
			if (powerType == POWER_MANA && target->GetTypeId() == TYPEID_PLAYER)
				pdamage -= ((Player*)target)->GetSpellCritDamageReduction(pdamage);

			uint32 gain = uint32(-target->ModifyPower(powerType, -pdamage));

			gain = uint32(gain * spellProto->EffectMultipleValue[GetEffIndex()]);

			// maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
			SpellNonMeleeDamage damageInfo(pCaster, target, spellProto->Id, SpellSchoolMask(spellProto->SchoolMask));
			pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

			damageInfo.target->CalculateAbsorbResistBlock(pCaster, &damageInfo, spellProto);

			pCaster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

			pCaster->SendSpellNonMeleeDamageLog(&damageInfo);

			// Set trigger flag
			uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
			uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
			uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE);
			if (damageInfo.damage)
				procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

			pCaster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, BASE_ATTACK, spellProto);

			pCaster->DealSpellDamage(&damageInfo, true);
			break;
		}
	case SPELL_AURA_MOD_REGEN:
		{
			// don't heal target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			int32 gain = target->ModifyHealth(m_modifier.m_amount);
			if (Unit* caster = GetCaster())
				target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f  * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
			break;
		}
	case SPELL_AURA_MOD_POWER_REGEN:
		{
			// don't energize target if not alive, possible death persistent effects
			if (!target->isAlive())
				return;

			Powers pt = target->getPowerType();
			if (int32(pt) != m_modifier.m_miscvalue)
				return;

			if (spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
			{
				// eating anim
				target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
			}
			else if (GetId() == 20577)
			{
				// cannibalize anim
				target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
			}

			// Anger Management
			// amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
			// so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
			if (pt == POWER_RAGE)
				target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
			break;
		}
		// Here tick dummy auras
	case SPELL_AURA_DUMMY:                              // some spells have dummy aura
	case SPELL_AURA_PERIODIC_DUMMY:
		{
			PeriodicDummyTick();
			break;
		}
	case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
		{
			TriggerSpell();
			break;
		}
	case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
		{
			TriggerSpellWithValue();
			break;
		}
	default:
		break;
	}
}

void Aura::PeriodicDummyTick()
{
	SpellEntry const* spell = GetSpellProto();
	Unit* target = GetTarget();
	switch (spell->SpellFamilyName)
	{
	case SPELLFAMILY_GENERIC:
		{
			switch (spell->Id)
			{
				// Forsaken Skills
			case 7054:
				{
					// Possibly need cast one of them (but
					// 7038 Forsaken Skill: Swords
					// 7039 Forsaken Skill: Axes
					// 7040 Forsaken Skill: Daggers
					// 7041 Forsaken Skill: Maces
					// 7042 Forsaken Skill: Staves
					// 7043 Forsaken Skill: Bows
					// 7044 Forsaken Skill: Guns
					// 7045 Forsaken Skill: 2H Axes
					// 7046 Forsaken Skill: 2H Maces
					// 7047 Forsaken Skill: 2H Swords
					// 7048 Forsaken Skill: Defense
					// 7049 Forsaken Skill: Fire
					// 7050 Forsaken Skill: Frost
					// 7051 Forsaken Skill: Holy
					// 7053 Forsaken Skill: Shadow
					return;
				}
			case 7057:                                  // Haunting Spirits
				if (roll_chance_i(33))
					target->CastSpell(target, m_modifier.m_amount, true, NULL, this);
				return;
				//              // Panda
				//              case 19230: break;
				//              // Gossip NPC Periodic - Talk
				//              case 33208: break;
				//              // Gossip NPC Periodic - Despawn
				//              case 33209: break;
				//              // Steal Weapon
				//              case 36207: break;
				//              // Simon Game START timer, (DND)
				//              case 39993: break;
				//              // Harpooner's Mark
				//              case 40084: break;
				//              // Old Mount Spell
				//              case 40154: break;
				//              // Magnetic Pull
				//              case 40581: break;
				//              // Ethereal Ring: break; The Bolt Burst
				//              case 40801: break;
				//              // Crystal Prison
				//              case 40846: break;
				//              // Copy Weapon
				//              case 41054: break;
				//              // Ethereal Ring Visual, Lightning Aura
				//              case 41477: break;
				//              // Ethereal Ring Visual, Lightning Aura (Fork)
				//              case 41525: break;
				//              // Ethereal Ring Visual, Lightning Jumper Aura
				//              case 41567: break;
				//              // No Man's Land
				//              case 41955: break;
				//              // Headless Horseman - Fire
				//              case 42074: break;
				//              // Headless Horseman - Visual - Large Fire
				//              case 42075: break;
				//              // Headless Horseman - Start Fire, Periodic Aura
				//              case 42140: break;
				//              // Ram Speed Boost
				//              case 42152: break;
				//              // Headless Horseman - Fires Out Victory Aura
				//              case 42235: break;
				//              // Pumpkin Life Cycle
				//              case 42280: break;
				//              // Brewfest Request Chick Chuck Mug Aura
				//              case 42537: break;
				//              // Squashling
				//              case 42596: break;
				//              // Headless Horseman Climax, Head: Periodic
				//              case 42603: break;
			case 42621:                                 // Fire Bomb
				{
					// Cast the summon spells (42622 to 42627) with increasing chance
					uint32 rand = urand(0, 99);
					for (uint32 i = 1; i <= 6; ++i)
					{
						if (rand < i * (i + 1) / 2 * 5)
						{
							target->CastSpell(target, spell->Id + i, true);
							break;
						}
					}
					break;
				}
				//              // Headless Horseman - Conflagrate, Periodic Aura
				//              case 42637: break;
				//              // Headless Horseman - Create Pumpkin Treats Aura
				//              case 42774: break;
				//              // Headless Horseman Climax - Summoning Rhyme Aura
				//              case 42879: break;
				//              // Tricky Treat
				//              case 42919: break;
				//              // Giddyup!
				//              case 42924: break;
				//              // Ram - Trot
				//              case 42992: break;
				//              // Ram - Canter
				//              case 42993: break;
				//              // Ram - Gallop
				//              case 42994: break;
				//              // Ram Level - Neutral
				//              case 43310: break;
				//              // Headless Horseman - Maniacal Laugh, Maniacal, Delayed 17
				//              case 43884: break;
				//              // Headless Horseman - Maniacal Laugh, Maniacal, other, Delayed 17
				//              case 44000: break;
				//              // Energy Feedback
				//              case 44328: break;
				//              // Romantic Picnic
				//              case 45102: break;
				//              // Romantic Picnic
				//              case 45123: break;
				//              // Looking for Love
				//              case 45124: break;
				//              // Kite - Lightning Strike Kite Aura
				//              case 45197: break;
				//              // Rocket Chicken
				//              case 45202: break;
				//              // Copy Offhand Weapon
				//              case 45205: break;
				//              // Upper Deck - Kite - Lightning Periodic Aura
				//              case 45207: break;
				//              // Kite -Sky  Lightning Strike Kite Aura
				//              case 45251: break;
				//              // Ribbon Pole Dancer Check Aura
				//              case 45390: break;
				//              // Holiday - Midsummer, Ribbon Pole Periodic Visual
				//              case 45406: break;
				//              // Alliance Flag, Extra Damage Debuff
				//              case 45898: break;
				//              // Horde Flag, Extra Damage Debuff
				//              case 45899: break;
				//              // Ahune - Summoning Rhyme Aura
				//              case 45926: break;
				//              // Ahune - Slippery Floor
				//              case 45945: break;
				//              // Ahune's Shield
				//              case 45954: break;
			case 45960:									// Nether Vapor Lightning
				target->CastSpell(target, m_modifier.m_amount, true, NULL, this);
				break;
				//              // Darkness
				//              case 45996: break;
			case 46041:                                 // Summon Blood Elves Periodic
				target->CastSpell(target, 46037, true, NULL, this);
				target->CastSpell(target, roll_chance_i(50) ? 46038 : 46039, true, NULL, this);
				target->CastSpell(target, 46040, true, NULL, this);
				return;
				//              // Transform Visual Missile Periodic
				//              case 46205: break;
				//              // Find Opening Beam End
				//              case 46333: break;
			case 46371:                                 // Ice Spear Control Aura
				target->CastSpell(target, 46372, true, NULL, this);
				return;
				//              // Hailstone Chill
				//              case 46458: break;
				//              // Hailstone Chill, Internal
				//              case 46465: break;
				//              // Chill, Internal Shifter
				//              case 46549: break;
				//              // Summon Ice Spear Knockback Delayer
				//              case 46878: break;
				//              // Send Mug Control Aura
				//              case 47369: break;
				//              // Direbrew's Disarm (precast)
				//              case 47407: break;
				//              // Mole Machine Port Schedule
				//              case 47489: break;
				//              // Mole Machine Portal Schedule
				//              case 49466: break;
				//              // Drink Coffee
				//              case 49472: break;
				//              // Listening to Music
				//              case 50493: break;
				//              // Love Rocket Barrage
				//              case 50530: break;
				// Exist more after, need add later
			default:
				break;
			}

			// Drink (item drink spells)
			if (GetEffIndex() > EFFECT_INDEX_0 && spell->EffectApplyAuraName[GetEffIndex()-1] == SPELL_AURA_MOD_POWER_REGEN)
			{
				if (target->GetTypeId() != TYPEID_PLAYER)
					return;
				// Search SPELL_AURA_MOD_POWER_REGEN aura for this spell and add bonus
				if (Aura* aura = GetHolder()->GetAuraByEffectIndex(SpellEffectIndex(GetEffIndex() - 1)))
				{
					aura->GetModifier()->m_amount = m_modifier.m_amount;
					((Player*)target)->UpdateManaRegen();
					// Disable continue
					m_isPeriodic = false;
					return;
				}
				return;
			}
			break;
		}
	case SPELLFAMILY_HUNTER:
		{
			// Aspect of the Viper
			switch (spell->Id)
			{
			case 34074:
				{
					if (target->GetTypeId() != TYPEID_PLAYER)
						return;
					// Should be manauser
					if (target->getPowerType() != POWER_MANA)
						return;
					Unit* caster = GetCaster();
					if (!caster)
						return;
					// Regen amount is max (100% from spell) on 21% or less mana and min on 92.5% or greater mana (20% from spell)
					int mana = target->GetPower(POWER_MANA);
					int max_mana = target->GetMaxPower(POWER_MANA);
					int32 base_regen = caster->CalculateSpellDamage(target, GetSpellProto(), m_effIndex, &m_currentBasePoints);
					float regen_pct = 1.20f - 1.1f * mana / max_mana;
					if (regen_pct > 1.0f) regen_pct = 1.0f;
					else if (regen_pct < 0.2f) regen_pct = 0.2f;
					m_modifier.m_amount = int32(base_regen * regen_pct);
					((Player*)target)->UpdateManaRegen();
					return;
				}
				//              // Knockdown Fel Cannon: break; The Aggro Burst
				//              case 40119: break;
			}
			break;
		}
	default:
		break;
	}

	if (Unit* caster = GetCaster())
	{
		if (target && target->GetTypeId() == TYPEID_UNIT)
			sScriptMgr.OnEffectDummy(caster, GetId(), GetEffIndex(), (Creature*)target, ObjectGuid());
	}
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
	if (!fearAuras.empty())
	{
		if (apply)
			GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGuid());
		else
			GetTarget()->SetFeared(true);
	}
}

void Aura::HandleManaShield(bool apply, bool Real)
{
	if (!Real)
		return;

	// prevent double apply bonuses
	if (apply && (GetTarget()->GetTypeId() != TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
	{
		if (Unit* caster = GetCaster())
		{
			float DoneActualBenefit = 0.0f;
			switch (GetSpellProto()->SpellFamilyName)
			{
			case SPELLFAMILY_MAGE:
				if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000008000))
				{
					// Mana Shield
					// +50% from +spd bonus
					DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto())) * 0.5f;
					break;
				}
				break;
			default:
				break;
			}

			DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

			m_modifier.m_amount += (int32)DoneActualBenefit;
		}
	}
}

void Aura::HandleArenaPreparation(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();

	target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION, apply);

	if (apply)
	{
		// max regen powers at start preparation
		target->SetHealth(target->GetMaxHealth());
		target->SetPower(POWER_MANA, target->GetMaxPower(POWER_MANA));
		target->SetPower(POWER_ENERGY, target->GetMaxPower(POWER_ENERGY));
	}
	else
	{
		// reset originally 0 powers at start/leave
		target->SetPower(POWER_RAGE, 0);
	}
}

void Aura::HandleAuraMirrorImage(bool apply, bool Real)
{
	if (!Real)
		return;

	// Target of aura should always be creature (ref Spell::CheckCast)
	Creature* pCreature = (Creature*)GetTarget();

	if (apply)
	{
		// Caster can be player or creature, the unit who pCreature will become an clone of.
		Unit* caster = GetCaster();

		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 0, caster->getRace());
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 1, caster->getClass());
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 2, caster->getGender());
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 3, caster->getPowerType());

		pCreature->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CLONED);

		pCreature->SetDisplayId(caster->GetNativeDisplayId());
	}
	else
	{
		const CreatureInfo* cinfo = pCreature->GetCreatureInfo();
		const CreatureModelInfo* minfo = sObjectMgr.GetCreatureModelInfo(pCreature->GetNativeDisplayId());

		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 0, 0);
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 2, minfo->gender);
		pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 3, 0);

		pCreature->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CLONED);

		pCreature->SetDisplayId(pCreature->GetNativeDisplayId());
	}
}

void Aura::HandleAuraSafeFall(bool Apply, bool Real)
{
	// implemented in WorldSession::HandleMovementOpcodes

	// only special case
	if (Apply && Real && GetId() == 32474 && GetTarget()->GetTypeId() == TYPEID_PLAYER)
		((Player*)GetTarget())->ActivateTaxiPathTo(506, GetId());
}

bool Aura::IsLastAuraOnHolder()
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (i != GetEffIndex() && GetHolder()->m_auras[i])
			return false;
	return true;
}

bool Aura::HasMechanic(uint32 mechanic) const
{
	return GetSpellProto()->Mechanic == mechanic ||
		GetSpellProto()->EffectMechanic[m_effIndex] == mechanic;
}

SpellAuraHolder::SpellAuraHolder(SpellEntry const* spellproto, Unit* target, WorldObject* caster, Item* castItem) :
	m_spellProto(spellproto),
	m_target(target), m_castItemGuid(castItem ? castItem->GetObjectGuid() : ObjectGuid()),
	m_auraSlot(MAX_AURAS), m_auraLevel(1),
	m_procCharges(0), m_stackAmount(1),
	m_timeCla(1000), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_AuraDRGroup(DIMINISHING_NONE),
	m_permanent(false), m_isRemovedOnShapeLost(true), m_deleted(false), m_in_use(0)
{
	MANGOS_ASSERT(target);
	MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry(spellproto->Id) && "`info` must be pointer to sSpellStore element");

	if (!caster)
		m_casterGuid = target->GetObjectGuid();
	else
	{
		// remove this assert when not unit casters will be supported
		MANGOS_ASSERT(caster->isType(TYPEMASK_UNIT))
			m_casterGuid = caster->GetObjectGuid();
	}

	m_applyTime      = time(NULL);
	m_isPassive      = IsPassiveSpell(spellproto);
	m_isDeathPersist = IsDeathPersistentSpell(spellproto);
	m_trackedAuraType= IsSingleTargetSpell(spellproto) ? TRACK_AURA_TYPE_SINGLE_TARGET : TRACK_AURA_TYPE_NOT_TRACKED;
	m_procCharges    = spellproto->procCharges;

	m_isRemovedOnShapeLost = (GetCasterGuid() == m_target->GetObjectGuid() &&
		m_spellProto->Stances &&
		!m_spellProto->HasAttribute(SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) &&
		!m_spellProto->HasAttribute(SPELL_ATTR_NOT_SHAPESHIFT));

	Unit* unitCaster = caster && caster->isType(TYPEMASK_UNIT) ? (Unit*)caster : NULL;

	m_duration = m_maxDuration = CalculateSpellDuration(spellproto, unitCaster);

	if (m_maxDuration == -1 || (m_isPassive && spellproto->DurationIndex == 0))
		m_permanent = true;

	if (unitCaster)
	{
		if (Player* modOwner = unitCaster->GetSpellModOwner())
			modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
	}

	// some custom stack values at aura holder create
	switch (m_spellProto->Id)
	{
		// some auras applied with max stack
	case 24575:                                         // Brittle Armor
	case 24659:                                         // Unstable Power
	case 24662:                                         // Restless Strength
	case 26464:                                         // Mercurial Shield
		m_stackAmount = m_spellProto->StackAmount;
		break;
	}

	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		m_auras[i] = NULL;
}

void SpellAuraHolder::AddAura(Aura* aura, SpellEffectIndex index)
{
	m_auras[index] = aura;
}

void SpellAuraHolder::RemoveAura(SpellEffectIndex index)
{
	m_auras[index] = NULL;
}

void SpellAuraHolder::ApplyAuraModifiers(bool apply, bool real)
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX && !IsDeleted(); ++i)
		if (Aura* aur = GetAuraByEffectIndex(SpellEffectIndex(i)))
			aur->ApplyModifier(apply, real);
}

void SpellAuraHolder::_AddSpellAuraHolder()
{
	if (!GetId())
		return;
	if (!m_target)
		return;

	// Try find slot for aura
	uint8 slot = NULL_AURA_SLOT;
	Unit* caster = GetCaster();

	// Lookup free slot
	// will be < MAX_AURAS slot (if find free) with !secondaura
	if (IsNeedVisibleSlot(caster))
	{
		if (IsPositive())                                   // empty positive slot
		{
			for (uint8 i = 0; i < MAX_POSITIVE_AURAS; i++)
			{
				if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
				{
					slot = i;
					break;
				}
			}
		}
		else                                                // empty negative slot
		{
			for (uint8 i = MAX_POSITIVE_AURAS; i < MAX_AURAS; i++)
			{
				if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
				{
					slot = i;
					break;
				}
			}
		}
	}

	// set infinity cooldown state for spells
	if (caster && caster->GetTypeId() == TYPEID_PLAYER)
	{
		if (m_spellProto->HasAttribute(SPELL_ATTR_DISABLED_WHILE_ACTIVE))
		{
			Item* castItem = m_castItemGuid ? ((Player*)caster)->GetItemByGuid(m_castItemGuid) : NULL;
			((Player*)caster)->AddSpellAndCategoryCooldowns(m_spellProto, castItem ? castItem->GetEntry() : 0, NULL, true);
		}
	}

	SetAuraSlot(slot);

	// Not update fields for not first spell's aura, all data already in fields
	if (slot < MAX_AURAS)                                   // slot found
	{
		SetAura(slot, false);
		SetAuraFlag(slot, true);
		SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
		UpdateAuraApplication();

		// update for out of range group members
		m_target->UpdateAuraForGroup(slot);

		UpdateAuraDuration();
	}

	//*****************************************************
	// Update target aura state flag (at 1 aura apply)
	// TODO: Make it easer
	//*****************************************************
	// Sitdown on apply aura req seated
	if (m_spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !m_target->IsSitState())
		m_target->SetStandState(UNIT_STAND_STATE_SIT);

	// register aura diminishing on apply
	if (getDiminishGroup() != DIMINISHING_NONE)
		m_target->ApplyDiminishingAura(getDiminishGroup(), true);

	// Update Seals information
	if (IsSealSpell(GetSpellProto()))
		m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);

	// Conflagrate aura state
	if (GetSpellProto()->IsFitToFamily(SPELLFAMILY_WARLOCK, UI64LIT(0x0000000000000004)))
		m_target->ModifyAuraState(AURA_STATE_CONFLAGRATE, true);

	// Faerie Fire (druid versions)
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_DRUID, UI64LIT(0x0000000000000400)))
		m_target->ModifyAuraState(AURA_STATE_FAERIE_FIRE, true);

	// Swiftmend state on Regrowth & Rejuvenation
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_DRUID, UI64LIT(0x0000000000000050)))
		m_target->ModifyAuraState(AURA_STATE_SWIFTMEND, true);

	// Deadly poison aura state
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_ROGUE, UI64LIT(0x0000000000010000)))
		m_target->ModifyAuraState(AURA_STATE_DEADLY_POISON, true);
}

void SpellAuraHolder::_RemoveSpellAuraHolder()
{
	// Remove all triggered by aura spells vs unlimited duration
	// except same aura replace case
	if (m_removeMode != AURA_REMOVE_BY_STACK)
		CleanupTriggeredSpells();

	Unit* caster = GetCaster();

	if (caster && IsPersistent())
		if (DynamicObject* dynObj = caster->GetDynObject(GetId()))
			dynObj->RemoveAffected(m_target);

	// remove at-store spell cast items (for all remove modes?)
	if (m_target->GetTypeId() == TYPEID_PLAYER && m_removeMode != AURA_REMOVE_BY_DEFAULT && m_removeMode != AURA_REMOVE_BY_DELETE)
		if (ObjectGuid castItemGuid = GetCastItemGuid())
			if (Item* castItem = ((Player*)m_target)->GetItemByGuid(castItemGuid))
				((Player*)m_target)->DestroyItemWithOnStoreSpell(castItem, GetId());

	// passive auras do not get put in slots - said who? ;)
	// Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
	// if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem()))
	//    return;

	uint8 slot = GetAuraSlot();

	if (slot >= MAX_AURAS)                                  // slot not set
		return;

	if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + slot)) == 0)
		return;

	// unregister aura diminishing (and store last time)
	if (getDiminishGroup() != DIMINISHING_NONE)
		m_target->ApplyDiminishingAura(getDiminishGroup(), false);

	SetAura(slot, true);
	SetAuraFlag(slot, false);
	SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

	m_procCharges = 0;
	m_stackAmount = 1;
	UpdateAuraApplication();

	if (m_removeMode != AURA_REMOVE_BY_DELETE)
	{
		// update for out of range group members
		m_target->UpdateAuraForGroup(slot);

		//*****************************************************
		// Update target aura state flag (at last aura remove)
		//*****************************************************
		uint32 removeState = 0;
		ClassFamilyMask removeFamilyFlag = m_spellProto->SpellFamilyFlags;
		switch (m_spellProto->SpellFamilyName)
		{
		case SPELLFAMILY_PALADIN:
			if (IsSealSpell(m_spellProto))
				removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
			break;
		case SPELLFAMILY_WARLOCK:
			if (m_spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000004)))
				removeState = AURA_STATE_CONFLAGRATE;   // Conflagrate aura state
			break;
		case SPELLFAMILY_DRUID:
			if (m_spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000400)))
				removeState = AURA_STATE_FAERIE_FIRE;   // Faerie Fire (druid versions)
			else if (m_spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000050)))
			{
				removeFamilyFlag = ClassFamilyMask(UI64LIT(0x00000000000050));
				removeState = AURA_STATE_SWIFTMEND;     // Swiftmend aura state
			}
			break;
		case SPELLFAMILY_ROGUE:
			if (m_spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000010000)))
				removeState = AURA_STATE_DEADLY_POISON; // Deadly poison aura state
			break;
		}

		// Remove state (but need check other auras for it)
		if (removeState)
		{
			bool found = false;
			Unit::SpellAuraHolderMap const& holders = m_target->GetSpellAuraHolderMap();
			for (Unit::SpellAuraHolderMap::const_iterator i = holders.begin(); i != holders.end(); ++i)
			{
				SpellEntry const* auraSpellInfo = (*i).second->GetSpellProto();
				if (auraSpellInfo->IsFitToFamily(SpellFamily(m_spellProto->SpellFamilyName), removeFamilyFlag))
				{
					found = true;
					break;
				}
			}

			// this has been last aura
			if (!found)
				m_target->ModifyAuraState(AuraState(removeState), false);
		}

		// reset cooldown state for spells
		if (caster && caster->GetTypeId() == TYPEID_PLAYER)
		{
			if (GetSpellProto()->HasAttribute(SPELL_ATTR_DISABLED_WHILE_ACTIVE))
				// note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
					((Player*)caster)->SendCooldownEvent(GetSpellProto());
		}
	}
}

void SpellAuraHolder::CleanupTriggeredSpells()
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
	{
		if (!m_spellProto->EffectApplyAuraName[i])
			continue;

		uint32 tSpellId = m_spellProto->EffectTriggerSpell[i];
		if (!tSpellId)
			continue;

		SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
		if (!tProto)
			continue;

		if (GetSpellDuration(tProto) != -1)
			continue;

		// needed for spell 43680, maybe others
		// TODO: is there a spell flag, which can solve this in a more sophisticated way?
		if (m_spellProto->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
			GetSpellDuration(m_spellProto) == int32(m_spellProto->EffectAmplitude[i]))
			continue;

		m_target->RemoveAurasDueToSpell(tSpellId);
	}
}

bool SpellAuraHolder::ModStackAmount(int32 num)
{
	uint32 protoStackAmount = m_spellProto->StackAmount;

	// Can`t mod
	if (!protoStackAmount)
		return true;

	// Modify stack but limit it
	int32 stackAmount = m_stackAmount + num;
	if (stackAmount > (int32)protoStackAmount)
		stackAmount = protoStackAmount;
	else if (stackAmount <= 0) // Last aura from stack removed
	{
		m_stackAmount = 0;
		return true; // need remove aura
	}

	// Update stack amount
	SetStackAmount(stackAmount);
	return false;
}

void SpellAuraHolder::SetStackAmount(uint32 stackAmount)
{
	Unit* target = GetTarget();
	Unit* caster = GetCaster();
	if (!target || !caster)
		return;

	bool refresh = stackAmount >= m_stackAmount;
	if (stackAmount != m_stackAmount)
	{
		m_stackAmount = stackAmount;
		UpdateAuraApplication();

		for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		{
			if (Aura* aur = m_auras[i])
			{
				int32 bp = aur->GetBasePoints();
				int32 amount = m_stackAmount * caster->CalculateSpellDamage(target, m_spellProto, SpellEffectIndex(i), &bp);
				// Reapply if amount change
				if (amount != aur->GetModifier()->m_amount)
				{
					aur->ApplyModifier(false, true);
					aur->GetModifier()->m_amount = amount;
					aur->ApplyModifier(true, true);
				}
			}
		}
	}

	if (refresh)
		// Stack increased refresh duration
			RefreshHolder();
}

Unit* SpellAuraHolder::GetCaster() const
{
	if (GetCasterGuid() == m_target->GetObjectGuid())
		return m_target;

	return ObjectAccessor::GetUnit(*m_target, m_casterGuid);// player will search at any maps
}

bool SpellAuraHolder::IsWeaponBuffCoexistableWith(SpellAuraHolder const* ref) const
{
	// only item casted spells
	if (!GetCastItemGuid())
		return false;

	// Exclude Debuffs
	if (!IsPositive())
		return false;

	// Exclude Non-generic Buffs and Executioner-Enchant
	if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC || GetId() == 42976)
		return false;

	// Exclude Stackable Buffs [ie: Blood Reserve]
	if (GetSpellProto()->StackAmount)
		return false;

	// only self applied player buffs
	if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetObjectGuid() != GetCasterGuid())
		return false;

	Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGuid());
	if (!castItem)
		return false;

	// Limit to Weapon-Slots
	if (!castItem->IsEquipped() ||
		(castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
		return false;

	// form different weapons
	return ref->GetCastItemGuid() && ref->GetCastItemGuid() != GetCastItemGuid();
}

bool SpellAuraHolder::IsNeedVisibleSlot(Unit const* caster) const
{
	bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem();

	for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
	{
		if (!m_auras[i])
			continue;

		// special area auras cases
		switch (m_spellProto->Effect[i])
		{
		case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
			return m_target != caster;
		case SPELL_EFFECT_APPLY_AREA_AURA_PET:
		case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
		case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
		case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
			// passive auras (except totem auras) do not get placed in caster slot
			return (m_target != caster || totemAura || !m_isPassive) && m_auras[i]->GetModifier()->m_auraname != SPELL_AURA_NONE;
		default:
			break;
		}
	}

	// passive auras (except totem auras) do not get placed in the slots
	return !m_isPassive || totemAura;
}

void SpellAuraHolder::HandleSpellSpecificBoosts(bool apply)
{
	uint32 spellId1 = 0;
	uint32 spellId2 = 0;
	uint32 spellId3 = 0;
	uint32 spellId4 = 0;

	switch (GetSpellProto()->SpellFamilyName)
	{
	case SPELLFAMILY_MAGE:
		{
			switch (GetId())
			{
			case 11189:                                 // Frost Warding
			case 28332:
				{
					if (m_target->GetTypeId() == TYPEID_PLAYER && !apply)
					{
						// reflection chance (effect 1) of Frost Ward, applied in dummy effect
						if (SpellModifier* mod = ((Player*)m_target)->GetSpellMod(SPELLMOD_EFFECT2, GetId()))
							((Player*)m_target)->AddSpellMod(mod, false);
					}
					return;
				}
			default:
				return;
			}
			break;
		}
	case SPELLFAMILY_WARRIOR:
		{
			if (!apply)
			{
				// Remove Blood Frenzy only if target no longer has any Deep Wound or Rend (applying is handled by procs)
				if (GetSpellProto()->Mechanic != MECHANIC_BLEED)
					return;

				// If target still has one of Warrior's bleeds, do nothing
				Unit::AuraList const& PeriodicDamage = m_target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
				for (Unit::AuraList::const_iterator i = PeriodicDamage.begin(); i != PeriodicDamage.end(); ++i)
					if ((*i)->GetCasterGuid() == GetCasterGuid() &&
						(*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARRIOR &&
						(*i)->GetSpellProto()->Mechanic == MECHANIC_BLEED)
						return;

				spellId1 = 30069;                           // Blood Frenzy (Rank 1)
				spellId2 = 30070;                           // Blood Frenzy (Rank 2)
			}
			else
				return;
			break;
		}
	case SPELLFAMILY_HUNTER:
		{
			switch (GetId())
			{
				// The Beast Within and Bestial Wrath - immunity
			case 19574:
			case 34471:
				{
					spellId1 = 24395;
					spellId2 = 24396;
					spellId3 = 24397;
					spellId4 = 26592;
					break;
				}
				// Misdirection, main spell
			case 34477:
				{
					if (!apply)
						m_target->getHostileRefManager().ResetThreatRedirection();
					return;
				}
			default:
				return; 
			}
			break;
		}
	default:
		return;
	}

	// prevent aura deletion, specially in multi-boost case
	SetInUse(true);

	if (apply)
	{
		if (spellId1)
			m_target->CastSpell(m_target, spellId1, true, NULL, NULL, GetCasterGuid());
		if (spellId2 && !IsDeleted())
			m_target->CastSpell(m_target, spellId2, true, NULL, NULL, GetCasterGuid());
		if (spellId3 && !IsDeleted())
			m_target->CastSpell(m_target, spellId3, true, NULL, NULL, GetCasterGuid());
		if (spellId4 && !IsDeleted())
			m_target->CastSpell(m_target, spellId4, true, NULL, NULL, GetCasterGuid());
	}
	else
	{
		if (spellId1)
			m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGuid());
		if (spellId2)
			m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGuid());
		if (spellId3)
			m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGuid());
		if (spellId4)
			m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGuid());
	}

	SetInUse(false);
}

SpellAuraHolder::~SpellAuraHolder()
{
	// note: auras in delete list won't be affected since they clear themselves from holder when adding to deletedAuraslist
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (Aura* aur = m_auras[i])
			delete aur;
}

void SpellAuraHolder::Update(uint32 diff)
{
	if (m_duration > 0)
	{
		m_duration -= diff;
		if (m_duration < 0)
			m_duration = 0;

		m_timeCla -= diff;

		if (m_timeCla <= 0)
		{
			if (Unit* caster = GetCaster())
			{
				Powers powertype = Powers(GetSpellProto()->powerType);
				int32 manaPerSecond = GetSpellProto()->manaPerSecond + GetSpellProto()->manaPerSecondPerLevel * caster->getLevel();
				m_timeCla = 1 * IN_MILLISECONDS;

				if (manaPerSecond)
				{
					if (powertype == POWER_HEALTH)
						caster->ModifyHealth(-manaPerSecond);
					else
						caster->ModifyPower(powertype, -manaPerSecond);
				}
			}
		}
	}

	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (Aura* aura = m_auras[i])
			aura->UpdateAura(diff);

	// Channeled aura required check distance from caster
	if (IsChanneledSpell(m_spellProto) && GetCasterGuid() != m_target->GetObjectGuid())
	{
		Unit* caster = GetCaster();
		if (!caster)
		{
			m_target->RemoveAurasByCasterSpell(GetId(), GetCasterGuid());
			return;
		}

		// need check distance for channeled target only
		if (caster->GetChannelObjectGuid() == m_target->GetObjectGuid())
		{
			// Get spell range
			float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

			if (Player* modOwner = caster->GetSpellModOwner())
				modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, NULL);

			if (!caster->IsWithinDistInMap(m_target, max_range))
			{
				caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
				return;
			}
		}
	}
}

void SpellAuraHolder::RefreshHolder()
{
	SetAuraDuration(GetAuraMaxDuration());
	UpdateAuraDuration();
}

void SpellAuraHolder::SetAuraMaxDuration(int32 duration)
{
	m_maxDuration = duration;

	// possible overwrite persistent state
	if (duration > 0)
	{
		if (!(IsPassive() && GetSpellProto()->DurationIndex == 0))
			SetPermanent(false);
	}
}

bool SpellAuraHolder::HasMechanic(uint32 mechanic) const
{
	if (mechanic == m_spellProto->Mechanic)
		return true;

	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (m_auras[i] && m_spellProto->EffectMechanic[i] == mechanic)
			return true;
	return false;
}

bool SpellAuraHolder::HasMechanicMask(uint32 mechanicMask) const
{
	if (mechanicMask & (1 << (m_spellProto->Mechanic - 1)))
		return true;

	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (m_auras[i] && m_spellProto->EffectMechanic[i] && ((1 << (m_spellProto->EffectMechanic[i] - 1)) & mechanicMask))
			return true;
	return false;
}

bool SpellAuraHolder::IsPersistent() const
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (Aura* aur = m_auras[i])
			if (aur->IsPersistent())
				return true;
	return false;
}

bool SpellAuraHolder::IsAreaAura() const
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (Aura* aur = m_auras[i])
			if (aur->IsAreaAura())
				return true;
	return false;
}

bool SpellAuraHolder::IsPositive() const
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (Aura* aur = m_auras[i])
			if (!aur->IsPositive())
				return false;
	return true;
}

bool SpellAuraHolder::IsEmptyHolder() const
{
	for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
		if (m_auras[i])
			return false;
	return true;
}

void SpellAuraHolder::UnregisterAndCleanupTrackedAuras()
{
	TrackedAuraType trackedType = GetTrackedAuraType();
	if (!trackedType)
		return;

	if (trackedType == TRACK_AURA_TYPE_SINGLE_TARGET)
	{
		if (Unit* caster = GetCaster())
			caster->GetTrackedAuraTargets(trackedType).erase(GetSpellProto());
	}

	m_trackedAuraType = TRACK_AURA_TYPE_NOT_TRACKED;
}

void SpellAuraHolder::SetAuraFlag(uint32 slot, bool add)
{
	uint32 index    = slot / 4;
	uint32 byte     = (slot % 4) * 8;
	uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
	val &= ~((uint32)AFLAG_MASK << byte);
	if (add)
	{
		if (IsPositive())
			val |= ((uint32)AFLAG_POSITIVE << byte);
		else
			val |= ((uint32)AFLAG_NEGATIVE << byte);
	}
	m_target->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void SpellAuraHolder::SetAuraLevel(uint32 slot, uint32 level)
{
	uint32 index    = slot / 4;
	uint32 byte     = (slot % 4) * 8;
	uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
	val &= ~(0xFF << byte);
	val |= (level << byte);
	m_target->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void SpellAuraHolder::UpdateAuraApplication()
{
	if (m_auraSlot >= MAX_AURAS)
		return;

	uint32 stackCount = m_procCharges > 0 ? m_procCharges * m_stackAmount : m_stackAmount;

	uint32 index    = m_auraSlot / 4;
	uint32 byte     = (m_auraSlot % 4) * 8;
	uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
	val &= ~(0xFF << byte);
	// field expect count-1 for proper amount show, also prevent overflow at client side
	val |= ((uint8(stackCount <= 255 ? stackCount - 1 : 255 - 1)) << byte);
	m_target->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void SpellAuraHolder::UpdateAuraDuration()
{
	if (GetAuraSlot() >= MAX_AURAS || m_isPassive)
		return;

	if (m_target->GetTypeId() == TYPEID_PLAYER)
	{
		WorldPacket data(SMSG_UPDATE_AURA_DURATION, 5);
		data << uint8(GetAuraSlot());
		data << uint32(GetAuraDuration());
		((Player*)m_target)->SendDirectMessage(&data);

		data.Initialize(SMSG_SET_EXTRA_AURA_INFO, (8 + 1 + 4 + 4 + 4));
		data << m_target->GetPackGUID();
		data << uint8(GetAuraSlot());
		data << uint32(GetId());
		data << uint32(GetAuraMaxDuration());
		data << uint32(GetAuraDuration());
		((Player*)m_target)->SendDirectMessage(&data);
	}

	// not send in case player loading (will not work anyway until player not added to map), sent in visibility change code
	if (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading())
		return;

	Unit* caster = GetCaster();

	if (caster && caster->GetTypeId() == TYPEID_PLAYER && caster != m_target)
		SendAuraDurationForCaster((Player*)caster);
}

void SpellAuraHolder::SendAuraDurationForCaster(Player* caster)
{
	WorldPacket data(SMSG_SET_EXTRA_AURA_INFO_NEED_UPDATE, (8 + 1 + 4 + 4 + 4));
	data << m_target->GetPackGUID();
	data << uint8(GetAuraSlot());
	data << uint32(GetId());
	data << uint32(GetAuraMaxDuration());                   // full
	data << uint32(GetAuraDuration());                      // remain
	caster->GetSession()->SendPacket(&data);
}
